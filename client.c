/**
Client Process:

Asks Collector about the data that have recived from the MasterWorker, takes a sequence of long by the command line.
For each long asks to the Collector if he recived the filenames whit a specific sum.
Client does a request for each long.

If Client is called without parameters in the command line, then  a special request is sent to the Collector, this request is for a list of sum|filename.
*/


#include "xerrori.h"

#define HOST "127.0.0.1"
#define PORT 65432 

int main(int argc, char *argv[]) {

  

  //TODO: request stuff from Collector 
  //TODO: check inputs by command line
  if(argc==1){ //client called without arguments in the commmand line
    // Send request 1 to Collector 
    // TODO: open socket with collector
    int skt_fd = 0;      // socket file descriptor 
    struct sockaddr_in serv_addr; // server address
    size_t e; //error checking

    // create socket 
    if ((skt_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
      fprintf(stderr,"Socket creation error. \n");
    
    // Address assigment
    serv_addr.sin_family = AF_INET;
    // port number must me converted in  network order
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);
    
    // Open connection
    if (connect(skt_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) 
      fprintf(stderr,"Error while opening connection. \n");

    //Send request Client 1
    char *req = "C1";
    int req_len = htonl(strlen(req));
    // First the length then the request
    e = writen(skt_fd,&req_len,sizeof(req_len));
    if(e!=sizeof(int)) fprintf(stderr,"Client Write Error (req 1). \n");
    e = writen(skt_fd,&req,ntohl(req_len)); //ntohl(req_len) = num bytes
    if(e!=ntohl(req_len)) fprintf(stderr,"Client Write Error (req 2). \n");

    //TODO: Read what the collector sent
    
    
    // Close connection
    if(close(skt_fd)<0)
      fprintf(stderr,"Error while closing socket.");
    
  }else{
    for(int i=1;i<argc;i++){
      int skt_fd = 0;      // socket file descriptor 
      struct sockaddr_in serv_addr; // server address
      size_t e; //error checking
  
      // create socket 
      if ((skt_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        fprintf(stderr,"Socket creation error. \n");
      
      // Address assigment
      serv_addr.sin_family = AF_INET;
      // port number must me converted in  network order
      serv_addr.sin_port = htons(PORT);
      serv_addr.sin_addr.s_addr = inet_addr(HOST);
      
      // Open connection
      if (connect(skt_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) 
        fprintf(stderr,"Error while opening connection. \n");
      
      // Send request 2 to Collector
      char *req = "C2";
      char *endptr;
      int req_len = htonl(strlen(req));
      // First the length then the request
      e = writen(skt_fd,&req_len,sizeof(req_len));
      if(e!=sizeof(int)) fprintf(stderr,"Client Write Error (req 2). \n");
      e = writen(skt_fd,&req,ntohl(req_len)); //ntohl(req_len) = num bytes
      if(e!=ntohl(req_len)) fprintf(stderr,"Client Write Error (req 2). \n");
      // Send every sum in the command linea
      int second_half = htonl(strtol(argv[i],&endptr,10));
      int first_half = htonl(strtol(argv[i],&endptr,10) >> 32);
      e = writen(skt_fd,&first_half,sizeof(first_half));
      if(e!=sizeof(int)) fprintf(stderr,"Client Write Error (sum). \n");
      e = writen(skt_fd,&second_half,sizeof(second_half));
      if(e!=sizeof(int)) fprintf(stderr,"Client Write Error (sum). \n");

      // Read what the collector sent
      long tot_sum;
      int ret_sum_h1,ret_sum_h2;
      char *fn;
      
      // read sum first
      e = readn(skt_fd,&ret_sum_h1,sizeof(ret_sum_h1));
      if(e!=sizeof(ret_sum_h1)) termina("Error from reading the response (sum half 1). \n");
      e = readn(skt_fd,&ret_sum_h2,sizeof(ret_sum_h2));
      if(e!=sizeof(ret_sum_h2)) termina("Error from reading the response (sum half 2). \n");
      // put the 2 halfs read together to get the total sum  
      tot_sum = ret_sum_h2;
      tot_sum = tot_sum << 32;
      tot_sum = tot_sum | ret_sum_h1;
      
      // read filename later
      e = readn(skt_fd,&fn,sizeof(fn));
      if(e!=sizeof(fn)) termina("Error from reading the response (fn). \n");

      //prints the couple filename and sum
      printf("Filename: %s, Sum: %ld", fn, tot_sum);
      
      // Close connection
      if(close(skt_fd)<0)
        fprintf(stderr,"Error while closing socket.");
    }
  }
  
  return 0;
}
