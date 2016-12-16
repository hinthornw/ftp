#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
// #include "sservices.h"

void dataConnect (int sock, struct sockaddr_in * client_addr, char * filename, char * opts);
int downloadFile(int port, char * filename, char * opts);

int main(int argc, char *argv[])
{
  int _PORT_U = 3059;
  return downloadFile(_PORT_U, "temp.jpg", "w");
}

int downloadFile(int port, char * filename, char * opts)
{
  int cfd, datafd, portno, pid;
  socklen_t sender_length;
  struct sockaddr_in rcv_addr, snd_addr;
  
  char * local_opts;
  if (filename == NULL) {
    fprintf(stderr,"Filename Null.\n");
    return -1;
  } 
  if (opts == NULL){
    local_opts = "w";
  } else{
    local_opts = opts;
  }
 
  datafd = socket(AF_INET, SOCK_STREAM, 0/*SOCK_NONBLOCK*/); // Use TCP connection, Not UDP
  if (datafd < 0) 
    perror("ERROR opening socket");
  bzero((char *) &rcv_addr, sizeof(rcv_addr)); // initialize receiver address
  portno = port;
  rcv_addr.sin_family = AF_INET;
  rcv_addr.sin_addr.s_addr = INADDR_ANY;
  rcv_addr.sin_port = htons(portno);

  if (bind(datafd, (struct sockaddr *) &rcv_addr,
          sizeof(rcv_addr)) < 0) 
          perror("ERROR on binding, must have authorized access.");

  listen(datafd,5); // 5 maximum queued requests
  printf("Listening on port %d.\n", ntohs(rcv_addr.sin_port));
  fflush(stdout);
  sender_length = sizeof(snd_addr);
  int i = 0;
  while (i++ < 5) {
     cfd = accept(datafd, 
           (struct sockaddr *) &snd_addr, &sender_length);
     if (cfd < 0) {
         perror("ERROR on accept");
         exit(1);
       }
     fflush(NULL);
     pid = fork();
     if (pid < 0)
         perror("ERROR on fork");
     else if (pid == 0)  { // in child
         printf("Server contacting from it's port %d\n", ntohs(snd_addr.sin_port));
         close(datafd);
         dataConnect(cfd, &snd_addr, filename, local_opts);
         exit(0);
     } else {
      printf("In Parent\n");
      fflush(stdout);
     //close(datafd); // in parent
     waitpid(pid, NULL, 0);
    }
  } /* end of while */
  close(datafd);
  return 0; /* we never get here */
}

void dataConnect (int sock,  struct sockaddr_in * client_addr, char * filename, char * opts)
{
  int maxfd;
  size_t n, BUFFER_SIZE = 1024;
  char buffer[BUFFER_SIZE];
  bzero(buffer,256);
  // enum State state = NEEDUSER; // initialize to need user
  fd_set rfds; // File descriptors to be read
  struct timeval tv; // Timer for read value
  int retval; // Return value for the timeval
  
  FILE * fp = fopen(filename, opts); // Make this dynamic..
  if(!fp) perror("Error creating file.");

  printf("In dataConnect.\n");


  //Initialize the to read FD.
  FD_ZERO(&rfds);
  FD_SET(sock, &rfds);
  maxfd = sock+1;
  //n = send(sock,"220 Service ready\r\n",19, MSG_CONFIRM);  
  int i = 0;
  while(i++ < 1000)
  {

    bzero(buffer,BUFFER_SIZE);
    tv.tv_sec = 20;
    tv.tv_usec = 0;
    retval = select(maxfd, &rfds, NULL, NULL, &tv);
    //fprintf(stderr, "Retval : %d\n", retval);
    if(retval == -1){fclose(fp); perror("select()");}
    else if (retval) {
        n = recv(sock,buffer, BUFFER_SIZE, 0);
        if (n < 0) perror("ERROR reading from socket");
        if(n != 0){
            //printf("Writing %lu bytes to file:\n%s\n",n, buffer); 
            if(fwrite(buffer, 1, n, fp) != n) fprintf(stderr, "Not %lu bytes written...\n", n);
        } 
    } else {
      fclose(fp);
      perror("Timeout");
    }
  }

  printf("Out of loop\n");
}