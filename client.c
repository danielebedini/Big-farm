/**
Client Process:

Asks Collector about the data recived from the MasterWorker, it takes a sequence of long by the command line.
For each long asks to the Collector if he recived the filenames whit a specific sum.
Client does a request for each long.

If Client is called without parameters in the command line, then  a special request is sent to the Collector, this request is for the complete list of (sum,filename).

*/

#include "xerrori.h"

#define HOST "127.0.0.1"
#define PORT 59643

int main(int argc, char *argv[]) {
  
  if(argc==1){  // TESTED and WORKING
    // client called without arguments in the commmand line
    // Send request 1 to Collector 
    
    int skt_fd = 0;      // socket file descriptor 
    struct sockaddr_in serv_addr; // server address
    size_t e; //error checking

    // create socket 
    if ((skt_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
      perror("Socket creation error. \n");
    
    // Address assigment
    serv_addr.sin_family = AF_INET;
    // port number must me converted in  network order
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);
    
    // Open connection
    if (connect(skt_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) 
      perror("Error while opening connection. \n");

    //Send request Client 1
    char req[2] = "C1";
    int req_len = htonl(strlen(req));
    
    // First the length then the request
    e = writen(skt_fd,&req_len,sizeof(req_len));
    if(e!=sizeof(req_len)) perror("Client Write Error (req 1). \n");
    e = writen(skt_fd,&req,ntohl(req_len)); //ntohl(req_len) = num bytes
    if(e!=ntohl(req_len)) perror("Client Write Error (req 2). \n");
    
    // First read how many things to read
    int len;
    e = readn(skt_fd,&len,sizeof(int));
    len = ntohl(len);
    if(e!=sizeof(int)) perror("Error from reading the response (len). \n");

    // check len 
    if(len==0){
      fprintf(stdout, "Nessun file.\n");
    }else{
      // store every couple in 2 arrays, one fur sums and one for filenames, both have same dimension (len)
      char *fn, *sum;
      int fn_len, sum_len;

      // then read every couple sent
      for(int i=0; i<len;i++){
        // read length sum
        e = readn(skt_fd,&sum_len,sizeof(int));
        sum_len = ntohl(sum_len);
        if(e!=sizeof(int)) perror("Error from reading sum length (C1).\n");
        // +1 for solving valgrind errors 
        sum = malloc((sum_len+1)*sizeof(char));
        e = readn(skt_fd,sum,sum_len);
        // \0 at the end of the string for solving valgrind errors 
        sum[sum_len]='\0';
        if(e!=sum_len) perror("Error from reading sum  (C1).\n");
        
        // read file length then filename 
        e = readn(skt_fd,&fn_len,sizeof(int));
        fn_len = ntohl(fn_len);
        if(e!=sizeof(int)) perror("Error from reading filename length (C1).\n");
        // +1 for solving valgrind errors 
        fn = malloc((fn_len+1)*sizeof(char));
        e = readn(skt_fd,fn,fn_len);
        // \0 at the end of the string for solving valgrind errors 
        fn[fn_len]='\0';
        if(e!=fn_len) perror("Error from reading filename length (C1).\n");
        
        fprintf(stdout,"Sum: %s\n", sum);
        fprintf(stdout,"File: %s\n", fn);

        // Free memory
        free(sum);
        free(fn);
      }
    }
    
    // Close connection
    if(close(skt_fd)<0)
      perror("Error while closing socket.\n");

  }
  else{
    char *endptr; // for using strtol
    // send request for every valid number
    for(int i=1;i<argc;i++){
      long sum=strtol(argv[i], &endptr,10);
      // open connection with socket 
      int skt_fd = 0;      // socket file descriptor 
      struct sockaddr_in serv_addr; // server address
      size_t e; //error checking
  
      // create socket 
      if ((skt_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        perror("Socket creation error. \n");
      
      // Address assigment
      serv_addr.sin_family = AF_INET;
      // port number must me converted in  network order
      serv_addr.sin_port = htons(PORT);
      serv_addr.sin_addr.s_addr = inet_addr(HOST);
      
      // Open connection
      if (connect(skt_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) 
        perror("Error while opening connection. \n");
  
      // Send request Client 2
      char req[2] = "C2";
      int req_len = htonl(strlen(req));

      // First the length then the request
      e = writen(skt_fd,&req_len,sizeof(req_len));
      if(e!=sizeof(req_len)) perror("Client Write Error (req 2). \n");
      e = writen(skt_fd,&req,ntohl(req_len)); //ntohl(req_len) = num bytes
      if(e!=ntohl(req_len)) perror("Client Write Error (req 2). \n");

      // send the valid sum
      int second_half = htonl(sum);
      int first_half = htonl(sum >> 32);
      e = writen(skt_fd,&first_half,sizeof(first_half));
      if(e!=sizeof(first_half)) perror("Write Error. \n");
      e = writen(skt_fd,&second_half,sizeof(second_half));
      if(e!=sizeof(second_half)) perror("Write Error. \n");
      
      // First read how many things to read
      int len;
      e = readn(skt_fd,&len,sizeof(int));
      len = ntohl(len);
      if(e!=sizeof(int)) perror("Error from reading the response (len C2). \n");

      // check len 
      if(len==0){
        fprintf(stdout, "Nessun file.\n");
      }else{
      
      // store every couple in 2 arrays, one fur sums and one for filenames, both have same dimension (len)
        char *fn, *sum;
        int fn_len, sum_len;
  
        // then read every couple sent
        for(int i=0; i<len;i++){
          // read length sum
          e = readn(skt_fd,&sum_len,sizeof(int));
          sum_len = ntohl(sum_len);
          if(e!=sizeof(int)) perror("Error from reading sum length (C2).\n");
          // +1 for solving valgrind errors 
          sum = malloc((sum_len+1)*sizeof(char));
          e = readn(skt_fd,sum,sum_len);
          // \0 at the end of the string for solving valgrind errors 
          sum[sum_len]='\0';
          if(e!=sum_len) perror("Error from reading sum  (C2).\n");
          
          // read file length then filename 
          e = readn(skt_fd,&fn_len,sizeof(int));
          fn_len = ntohl(fn_len);
          if(e!=sizeof(int)) perror("Error from reading filename length (C2).\n");
          // +1 for solving valgrind errors 
          fn = malloc((fn_len+1)*sizeof(char));
          e = readn(skt_fd,fn,fn_len);
          // \0 at the end of the string for solving valgrind errors 
          fn[fn_len]='\0';
          if(e!=fn_len) perror("Error from reading filename length (C2).\n");
          
          fprintf(stdout,"Sum: %s\n", sum);
          fprintf(stdout,"File: %s\n", fn);
  
          // Free memory
          free(sum);
          free(fn);
        }
      }
    }
    return 0;
  }
}