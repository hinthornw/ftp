/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
      5.1.  MINIMUM IMPLEMENTATION

      In order to make FTP workable without needless error messages, the
      following minimum implementation is required for all servers:

         TYPE - ASCII Non-print
         MODE - Stream
         STRUCTURE - File, Record
         COMMANDS - USER, QUIT, PORT,
                    TYPE, MODE, STRU,
                      for the default values
                    RETR, STOR,
                    NOOP.

      The default values for transfer parameters are:

         TYPE - ASCII Non-print
         MODE - Stream
         STRU - File

      All hosts must accept the above as the standard defaul
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

const int TELNET =  21;
void dataConnect(int,int, struct sockaddr_in *); /* function prototype */
void handler(int n);
int login(int, char *);
void error(const char *msg)
{
    perror(msg);
    exit(1);
}
//typedef void (*sighandler_t)(int);
//signhandler_t signal(int sig, sighandler_t handler);

int checkType(){}
int checkMode(){}
int checkStruct(){}
int checkUSR(){}
int checkAUTH(){}




int main(int argc, char *argv[])
{
  int controlfd, datafd, portno, pid;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  //Set up process to handle zombie children
  if (signal(SIGCHLD, handler) == SIG_ERR){ 
    fprintf(stderr, "Cannot set up CHLD signal handler\n");
  }


  controlfd = socket(AF_INET, SOCK_STREAM, 0); // Use TCP connection, Not UDP
  if (controlfd < 0) 
    error("ERROR opening socket");

  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = TELNET;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(controlfd, (struct sockaddr *) &serv_addr,
          sizeof(serv_addr)) < 0) 
          error("ERROR on binding, must have authorized access.");

  listen(controlfd,5); // 5 maximum queued requests
  printf("Listening on port %d.\n", ntohs(serv_addr.sin_port));
  fflush(stdout);
  clilen = sizeof(cli_addr);
  while (1) {
     datafd = accept(controlfd, 
           (struct sockaddr *) &cli_addr, &clilen);
     if (datafd < 0) 
         error("ERROR on accept");
     pid = fork();
     if (pid < 0)
         error("ERROR on fork");
     if (pid == 0)  { // in child
         close(controlfd);
         dataConnect(datafd,controlfd, &cli_addr);
         exit(0);
     }
     else close(datafd); // in parent
  } /* end of while */
  close(controlfd);
  return 0; /* we never get here */
}

/******** dataConnect() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
// typedef struct dict{
//   char usr = "USER";
//   char psw[] = "PASS";



// } msg;




enum State{NEEDUSER, NEEDPASS, LOGGEDIN, CLOSED};


void dataConnect (int sock, int controlfd, struct sockaddr_in * client_addr)
{
  int n, maxfd;
  char buffer[1024];
  bzero(buffer,256);
  enum State state = NEEDUSER; // initialize to need user
  int usr = 0;
  int pw = 0;
  fd_set rfds; // File descriptors to be read
  struct timeval tv; // Timer for read value
  int retval, ret; // Return value for the timeval
  printf("In dataConnect.\n");

  //Initialize the to read FD.
  FD_ZERO(&rfds);
  FD_SET(sock, &rfds);
  maxfd = sock+1;

  n = send(sock,"220 Service ready\r\n",53, MSG_CONFIRM);  
  while(1)
  {
    // USER, QUIT, PORT,
    //                 TYPE, MODE, STRU,
    //                   for the default values
    //                 RETR, STOR,
    //                 NOOP.
    printf("In loop\n");
    if(state == NEEDUSER){
      ret = login(sock, buffer);
      if(ret){
        error("LOGINFAIL");
      }else{
        state = LOGGEDIN;
      }
    }

    bzero(buffer,1024);
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    retval = select(maxfd, &rfds, NULL, NULL, &tv);
    if(retval == -1)error("select()");
    else if (retval) {
        n = recv(sock,buffer,1024, 0);
        if (n < 0) error("ERROR reading from socket");
        if(n != 0){
            printf("%d: %s\n",n, buffer);   
        } 
    } else {
      error("Timeout");
    }
    //User is logged in
    if (state != NEEDUSER){
      if (strncmp(buffer, "TYPE", 4) == 0){
        printf("TYPE received\n");

       }
       else if (strncmp(buffer, "MODE", 4) == 0){
        printf("MODE received\n");
       }
       else if (strncmp(buffer, "STRU", 4) == 0){
        printf("STRU received\n");
       }
       else if (strncmp(buffer, "RETR", 4) == 0){ //retrieve
        printf("RETR received\n");
        n = send(sock,"150 File status OK; about to open data connection\r\n",53, MSG_CONFIRM);
        //MAKE DATA CONNECTION TO THE CORRECT PORT!!
        // if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        //   error("ERROR connecting");
       }
       else if (strncmp(buffer, "STOR", 4) == 0){
        printf("STOR received\n");
       }
       else if (strncmp(buffer, "NOOP", 4) == 0){
          printf("NOOP received\n");
       } 

       bzero(buffer, sizeof(buffer));
   }
  }
    
 

  printf("Closing control socket\n");
  close(controlfd);
  printf("Out of loop\n");
}

void handler(int n){
  signal(SIGCHLD,SIG_IGN);
}


int login(int sock, char * buffer){
  int n, maxfd;
  enum State state = NEEDUSER; // initialize to need user
  int usr = 0;
  int pw = 0;
  fd_set rfds; // File descriptors to be read
  struct timeval tv; // Timer for read value
  int retval; // Return value for the timeval
  printf("In login\n");
  //Initialize the to read FD.
  FD_ZERO(&rfds);
  FD_SET(sock, &rfds);
  maxfd = sock+1;
  while(1){
    if(state == NEEDUSER){
      printf("Need Username\n");
    }else if (state == NEEDPASS){
      printf("Need Password\n");
    }
    bzero(buffer,1024);
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    retval = select(maxfd, &rfds, NULL, NULL, &tv);
    if(retval == -1)error("select()");
    else if (retval)
    {
        n = recv(sock,buffer,1024, 0);
        if (n < 0) error("ERROR reading from socket");
        if(n != 0){
            printf("%d: %s\n",n, buffer);   
        } 
  } else {
      printf("Timeout\n");
      return 1;
  }

  if(state == NEEDUSER && strncmp(buffer, "USER", 4) == 0){ // User signin
      printf("Received Username\n");
      if(state != NEEDUSER) {error("Error: Username already input.\n");}
      else{
        n = send(sock,"331 User name ok, need password\r\n",35, MSG_CONFIRM);
        if (n < 0) error("ERROR writing to socket");
        state = NEEDPASS;
      }
     }
  else if (state == NEEDPASS && strncmp(buffer, "PASS", 4) == 0){ //password
      printf("Received Password\n");
      if(state!= NEEDPASS){
        error("Error: Password input out of order.\n");
      } else{
        n = send(sock,"230 User logged in\r\n",22, MSG_CONFIRM);
        if (n < 0) error("ERROR writing to socket");
        return 0;
      }
  } else{
      printf("Not pass or user. Exiting.\n");
      return 1;
  }
  }
}