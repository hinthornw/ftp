#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include "sservices.h"


const int _PORT_L  = 21; // TODO change this..

void servProcess(int, struct sockaddr_in *); /* function prototype */

int main(int argc, char *argv[])
{
  int controlfd,  clientfd, pid;
  socklen_t clilen;
  struct sockaddr_in serv_ctrl_addr,  cli_addr;
 
  //Set up process to handle zombie children
  if (signal(SIGCHLD,SIG_IGN) == SIG_ERR){ 
    fprintf(stderr, "Cannot set up CHLD signal handler\n");
  }
  if(signal(SIGTERM, handler) == SIG_ERR){
    fprintf(stderr, "Cannot set up TERM signal handler\n");
  }

  controlfd = socket(AF_INET, SOCK_STREAM, 0); // Use TCP connection, Not UDP
  if (controlfd < 0) 
    error("Control Socket");
  int optval = 1;
  if(setsockopt(controlfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) 
    perror("setsockopt()");
  //Bind Control Socket
  bzero((char *) &serv_ctrl_addr, sizeof(serv_ctrl_addr));
  serv_ctrl_addr.sin_family = AF_INET;
  serv_ctrl_addr.sin_addr.s_addr = INADDR_ANY;
  serv_ctrl_addr.sin_port = htons(_PORT_L);
  if (bind(controlfd, (struct sockaddr *) &serv_ctrl_addr,
          sizeof(serv_ctrl_addr)) < 0) 
          error("ERROR on binding, must have authorized access.");

  listen(controlfd,5); // 5 maximum queued requests
  #ifdef DEBUG
  printf("Listening on port %d.\n", ntohs(serv_ctrl_addr.sin_port));
  #endif

  fflush(stdout);
  clilen = sizeof(cli_addr);
  while (1) {
     clientfd = accept(controlfd, 
           (struct sockaddr *) &cli_addr, &clilen);
     if (clientfd < 0) 
         error("ERROR on accept");
     fflush(NULL);
     pid = fork();
     if (pid < 0)
         error("ERROR on fork");
     else if (pid == 0)  { // in child
         close(controlfd);
         servProcess(clientfd, &cli_addr);
         exit(0);
     }
     else close(clientfd); // in parent
  } /* end of while */
  close(controlfd);
  return 0; /* we never get here */
}





// } msg;
    // USER, QUIT, PORT,
    //                 TYPE, MODE, STRU,
    //                   for the default values
    //                 RETR, STOR,
    //                 NOOP.
    // Need to have the client always be able to retrieve input from stdin
    // when not busy i.e.:
    // At startup, create TWO sockets -> control and data
    // Only attempt to bind data if PORT command set
    // ftp connect to server->
    // can then do Retr [file]
      //   -> servProcess
      //   -> Downloadfile
      //   -> return to user control
      // STOR file
      //   -> servProcess
      //   ->uploadfile
      //   ->return to user control
      // MODE 
      //   -> Set state variables...
      //   -> need a "MODE" type with stream variable (to be expanded later)
      // Structure
      //   -> set state variables (i think?)
      //   -> File & record...
      // USER
      //   -> login (NEED TO TWEAK)
      // QUIT
      //   -> Send ABOR message
      //   -> Close connection
      // PORT -> from command line OR through later command
      //   -> set state variables 
      //   -> BIND socket to particular portno





//State:  NEEDUSER, NEEDPASS, LOGGEDIN
//Switch(state):
  // case NEEDUSER: 
  //       ret = login(bla);
  //       if(-1) send(sock, "hmm Username not valid?\r\n",no, MSG_CONFIRM);
  //       else(
  //           send(sock, "230 User logged in\r\n", 20, MSG_CONFIRM);
  //           continue
  //         )
  // case LOGGEDIN:
  //       {
  //         retval = select(maxfd, &rfds, &wfds, NULL & tv);
  //         if(ret == -1)error("select()");
  //         else if(retval){
  //           //find out if there is something to read, if so do it, else
  //           //if there is something to write, then do that
  //           //
  //           a) buffer the stuff,
  //           b) parse the buffer
  //           c) determine actions
  //           -> b & c could be in separate function
  //           yeah
  //         }
  //       }
  // default:
  //       send(sock, "500 CASE not accounted for. Cutting Connection");
// handle input(char * input, COMMAND * command, char * buffer);


/******** servProcess() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/

void servProcess (int sock, struct sockaddr_in * client_addr)
{
  int n, maxfd;
  int BUFFERSIZE = 1024;
  char buffer[BUFFERSIZE];
  bzero(buffer,BUFFERSIZE);
  enum servState state = NEEDUSER; // initialize to need user
  fd_set rfds; // File descriptors to be read
  struct timeval tv; // Timer for read value
  int retval, ret; // Return value for the timeval
  int datafd;
  //struct sockaddr_in serv_data_addr;
  Session sesh;
  Tokens tokens; // string parsing
  size_t numtokens; //parsed in 

   /***Assign default variables for this session ***/
  bzero((char *) &sesh, sizeof(sesh));
  sesh.serv_data_portno = _PORT_L - 1;
  sesh.mode = STREAM;
  sesh.type = ASCIINP;
  sesh.structure = sFILE;
  sesh.c_control = *client_addr;
  sesh.c_control.sin_addr =  client_addr->sin_addr;
  sesh.c_data =  *client_addr;
  sesh.c_data.sin_addr = client_addr->sin_addr;
  // #ifdef DEBUG
  // printf("LIST OF ADDRESSES\n%p\n%p\n%p\n", &(client_addr->sin_addr.s_addr), &sesh.c_control.sin_addr.s_addr, &sesh.c_data.sin_addr.s_addr);
  // printf("LIST OF VLUES\n%u\n%u\n%u\n", client_addr->sin_addr.s_addr, sesh.c_control.sin_addr.s_addr, sesh.c_data.sin_addr.s_addr);
  // #endif

  // sesh.c_control->sin_family = AF_INET;

  //   bcopy((char *)client_addr->sin_addr.s_addr, 
  //        (char *)&(sesh.c_control->sin_addr.s_addr),
  //        sizeof(uint32_t));
  //   bcopy((char *)client_addr->)
  //   serv_addr->sin_port = htons(_PORT_L);

  /***              End Assign                             ***/

  // /***  Create a data socket (already have control socket) ***/
  // datafd = socket(AF_INET, SOCK_STREAM, 0);
  // if (datafd < 0){
  //   error("Data Socket");
  // }
  // bzero((char *) &serv_data_addr, sizeof(serv_data_addr));
  // serv_data_addr.sin_family = AF_INET;
  // serv_data_addr.sin_addr.s_addr = INADDR_ANY;
  // serv_data_addr.sin_port = htons(_PORT_L - 1);
  // if (bind(datafd, (struct sockaddr *) &serv_data_addr,
  //         sizeof(serv_data_addr)) < 0) 
  //         error("ERROR on binding, must have authorized access.");
  /*** Wait, what...                                      ***/
  #ifdef DEBUG
  printf("In dataConnect.\n");
  #endif

 

  maxfd = sock+1;//(sock > datafd) sock + 1 ? datafd + 1;
  while(1)
  {
    #ifdef DEBUG
    fprintf(stderr,"In loop\n");
    fflush(stderr);
    #endif
     //Initialize the to read FD.
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    bzero(buffer,BUFFERSIZE);
    switch(state){
      case NEEDUSER:
          #ifdef DEBUG
          printf("NEED USER = %d\n", state);
          #endif
          n = send(sock,"220 Service ready\r\n",19, MSG_CONFIRM);
          if (n < 0){
            error("send()");
          }
          ret = login(&sesh, sock, buffer);
          if(ret < 0){
            error("Login Fail");
          } else{
            state = LOGGEDIN;
            break;
          }
      case LOGGEDIN:
        printf("LOOGED IN = %d\n", state);

        /**** Wait up to 30 seconds for a message from client ***/
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        retval = select(maxfd, &rfds, NULL, NULL, &tv);
        if(retval == -1){
          //close(datafd);
          error("select()");
        }
        else if (retval) {
            n = recv(sock,buffer,BUFFERSIZE - 1, 0);
            if (n < 0) error("ERROR reading from socket");
            else if (n == 0) continue;
            else{
              tokens = strsplit(buffer, ", \t\r\n", &numtokens); // PARSE INPUT
              #ifdef DEBUG
              fprintf(stderr, "%d: Command: %s Token: %lu\n", n, tokens[0], numtokens); 
              fflush(stderr);
              for(size_t j = 1; j < numtokens; j++){
                fprintf(stderr, "%lu: \"%s\"\n", j, tokens[j]);
              }
              #endif
            } 
            
        } else {
          //close(datafd);
          error("Timeout");
        }
        /*** Message received and parsed. Handle message ***/
        
        
        switch(checkMessage(sock, tokens)){
            case INVALID:
              break;              
            case QUIT:
              #ifdef DEBUG
              printf("QUIT received. Closing connections.\n");
              #endif
              freeTokens(tokens, numtokens);
              close(sock);
              close(datafd);
              exit(0);
            case PORT:
              if(changePort(&sesh, tokens) < 0) error("changePort()");
              else
                 n = send(sock,"200 Port List Change\r\n",22, MSG_CONFIRM);
              break;
            case TYPE:
              //n = send(sock, "", , MSG_CONFIRM);
              break;
            case MODE:
              // set to stream, block, compressed
              break;
            case STRU:
              //set STR value {file or record}
              break;
            case RETR: //Connect data stream to client, xfer from server to client using current parameters
              datafd = connect_to_client(&sesh);
              // transferFile(Session, char * filename, int sDataSocket, int control);
              if(transferFile(&sesh, tokens[1], datafd, sock) < 0) {
                freeTokens(tokens, numtokens);
                close(datafd);
              }
              break;
            case STOR://connect data strem to client, xfer from client ot server using current parameters
              break;

            case NOOP:
              n = send(sock,"220 Service ready\r\n",19, MSG_CONFIRM);
              if (n < 0){
                error("send()");
              }   
              break;           
            default:
              #ifdef DEBUG
              fprintf(stderr,"Default Command case reached. Check\n");
              fflush(stderr);
              #endif
              break;
        };
        break;
       

        default:
          #ifdef DEBUG
          fprintf(stderr,"Default state reached (%d). Check\n", state);
          fflush(stderr);
          #endif
          continue;
        

    }
  }
    
 
  printf("Out of loop\n");
}


 // tokens = strsplit(line, ", \t\n", &numtokens);
        // for (size_t i = 0; i < numtokens; i++) {
        //     printf("    token: \"%s\"\n", tokens[i]);
        //     free(tokens[i]);
        // }
        // if (tokens != NULL)
        //     free(tokens);