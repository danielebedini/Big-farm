#include "xerrori.h"

// localhost address and a port for the socket
#define HOST "127.0.0.1"
#define PORT 59643

int qlen=8; // default buffer size
extern char *optarg;  // for using getopt(3)   
volatile sig_atomic_t sig_arrived = 0; // for using SIGINT

typedef struct { 
  char **buffer;              // buffer shared between prod and cons   
  int *cindex;                // buffer index
  pthread_mutex_t *cmutex;    // mutex 
  sem_t *sem_free_slots;      // semaphore for free slots
  sem_t *sem_data_items;      // semaphore for data items
} wdata;

/* 
Thread Worker body:

This threads are launched by the MasterWorker process, each thread reads a file name in the buffer given by th Master, calculates the total sum of long numbers in given file.
Then every thread creates a socket with the server Collector.py and sends the sum.
*/

void *wtbody(void * data){
  
  wdata *wd= (wdata *) data;
  char *fn;
  long tot_sum=0;
  
  while(true){
    // reads filename from buffer, then increments the index by 1
    xsem_wait(wd->sem_data_items,__LINE__,__FILE__);
	  xpthread_mutex_lock(wd->cmutex,__LINE__,__FILE__);
    fn = wd->buffer[*(wd->cindex) % qlen];
    *(wd->cindex) +=1;
  	xpthread_mutex_unlock(wd->cmutex,__LINE__,__FILE__);
    xsem_post(wd->sem_free_slots,__LINE__,__FILE__);
    
    // check if thread must end
    if(!strcmp(fn,"-1")){
      break;
    }
    
  // reads the file read from the buffer and calculates the
    FILE *f = fopen(fn,"rb");
    if(f!=NULL){
      int i=0;
      long num;
      do{
        size_t e = fread(&num,sizeof(num),1,f);
        if(e!=1) break;
        // sums(i*file[i]) from i=0 to n=long numbers in given file
        tot_sum+=i*num;
        i++;
      }while(true);
    }else{
      fprintf(stderr,"No such file (%s) in this directory.\n", fn);
      continue;
    }
    fclose(f);
    
    // Gives the sum to the Collector
    int skt_fd;      // socket file descriptor 
    struct sockaddr_in serv_addr; // server address
    size_t e; // error checking

    // create socket 
    if ((skt_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
      perror("Socket creation error (MW). \n");
    
    // Address assigment
    serv_addr.sin_family = AF_INET;
    // port number must me converted in  network order
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);
    
    // Open connection
    if (connect(skt_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
      perror("Error while opening connection (MW). \n");
    
    // MW Request
    char req[2] = "MW";
    int req_len = htonl(strlen(req));
    
    // write total sum in the socket, one half at a time.
    int second_half = htonl(tot_sum);
    int first_half = htonl(tot_sum >> 32);

    // first send the request
    e = writen(skt_fd,&req_len,sizeof(req_len));
    if(e!=sizeof(int)) perror("Write Error req_len (MW). \n");
    e = writen(skt_fd,&req,htonl(req_len)); //ntohl(req_len) = num bytes
    if(e!=htonl(req_len)) perror("Write Error req (MW). \n");
    
    // then send the sum
    e = writen(skt_fd,&first_half,sizeof(first_half));
    if(e!=sizeof(first_half)) perror("Write Error first-half (MW). \n");
    e = writen(skt_fd,&second_half,sizeof(second_half));
    if(e!=sizeof(second_half)) perror("Write Error second-half (MW). \n");
    
    // then send file name, first the length, then the name
    int f_len = htonl(strlen(fn));
    e = writen(skt_fd,&f_len,sizeof(f_len));
    if(e!=sizeof(f_len)) perror("Write Error f_len (MW). \n");
    e = writen(skt_fd,fn,ntohl(f_len)); //ntohl(f_len) = num bytes
    if(e!=ntohl(f_len)) perror("Write Error fn (MW). \n");

    // Close connection
    if(close(skt_fd)<0)
      perror("Error while closing socket (MW).\n");
  }

  pthread_exit(NULL);
}


/*
MasterWorker body (exe: farm):

This is the main process of the project, it handles the SIGINT signal, checks the input given by the user from the command line, then launches the Worker threads.

*/

void sig_handler(int sig){
  if(sig==SIGINT) sig_arrived=1;
}

int main(int argc, char *argv[]) {
  
  // check args
  if (argc < 2) {
    printf("Uso: %s file [file ...]\n", argv[0]);
    return 1;
  } 
  
  //Handling signal SIGINT
  struct sigaction sig_action;
  sigaction(SIGINT, NULL, &sig_action);
  sig_action.sa_handler=sig_handler;
  sigaction(SIGINT, &sig_action, NULL);
  //nt
  int opt; // for using getopt() 
  int tn=4, del=0, on=0; // default values
  char *endptr;
  while((opt = getopt(argc,argv,"n:q:t:"))!=-1){
    switch (opt){ 
      case 'n':
        // threads number
        tn = atoi(optarg);
        on++;
        assert(tn>0);
      break;
      case 'q':
        // buffer length
        qlen=atoi(optarg);
        on++;
        assert(qlen>0);
      break;
      case 't':
        // prod/cons delay
        strtol(optarg,&endptr,10);
        if(endptr==optarg){
          fprintf(stderr,"Wrong Delay.\n");
          return 0;
        }else{
          del=atoi(optarg);
          on++;
        }
        assert(del>=0);
      break;
    }
  }

  // initializations: buffer, threads and semaphores
  char *buffer[qlen]; // shared buffer
  int pindex=0,cindex=0; // prod and con index
  pthread_mutex_t cmutex = PTHREAD_MUTEX_INITIALIZER; // mutex
  pthread_t wt[tn]; // worker threads
  wdata wdata[tn]; // data for the worker threads
  // semaphores for prod and cons
  sem_t sem_free_slots, sem_data_items; 
  xsem_init(&sem_free_slots,0,qlen,__LINE__,__FILE__);
  xsem_init(&sem_data_items,0,0,__LINE__,__FILE__);
  
  // worker's threads start -> nt = number of threads
  for(int i=0;i<tn;i++) {
    wdata[i].buffer = buffer;
		wdata[i].cindex = &cindex;
		wdata[i].cmutex = &cmutex;
    wdata[i].sem_data_items = &sem_data_items;
    wdata[i].sem_free_slots = &sem_free_slots;
    xpthread_create(&wt[i],NULL,wtbody,&wdata[i],__LINE__,__FILE__);
  }
  
  // MW puts everything typed in the command line into the buffer
  for(int i=2*on+1; i<argc && !sig_arrived; i++){ //for stops if sig_arrived is 1
    // producer puts filename in the buffer  
    xsem_wait(&sem_free_slots,__LINE__,__FILE__);
    buffer[pindex++ % qlen] = argv[i];
    xsem_post(&sem_data_items,__LINE__,__FILE__); 
    usleep(del*1000); // delay between a write and another
  }
  
  // stop the threads
  for(int i=0;i<tn;i++) {
    xsem_wait(&sem_free_slots,__LINE__,__FILE__);
    buffer[pindex++ % qlen] = "-1";
    xsem_post(&sem_data_items,__LINE__,__FILE__);
  } 
  
  // thread join
  for(int i=0;i<tn;i++) {
    xpthread_join(wt[i],NULL,__LINE__,__FILE__);
  }

  // destroy mutex and semaphores
  pthread_mutex_destroy(&cmutex);
  sem_destroy(&sem_data_items);
  sem_destroy(&sem_free_slots); 
  
  return 0;
}