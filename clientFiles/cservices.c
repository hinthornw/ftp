#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <dirent.h>
// #include <stddef.h>
#include "cservices.h"



// void dataConnect (int sock, struct sockaddr_in * client_addr, char * filename);
// int downloadFile(int control, int port, char * filename);
// int port(int control, int * port, struct sockaddr_in  * snd_addr);
// int handleCommand(enum cmdType cmd, int control, int * datafd, struct sockaddr_in  * snd_addr_data, Tokens * toks, size_t * numtokens, enum State * state);
// int getCommand(char * buffer, int size, Tokens * toks, size_t * numtokens);
// enum cmdType commandType(char * in);
// char * getIpAddress();
// Tokens strsplit(const char* str, const char* delim, size_t* numtokens);

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int asciinpRead(char * buffer, const int BUFFER_SIZE, FILE * stream){
  int c;  
  int i = 0;
  for(; i < BUFFER_SIZE -1; i++){
    c = fgetc(stream);
    if(c == '\n'){
      buffer[i++] = '\r';
      buffer[i] = '\n';
      //break;
    }else if (feof(stream)){
      fprintf(stderr, "EOF\n");
      break;
    } else if (ferror(stream)){
        perror("getc()");
        return -1;
    } else{
      buffer[i] = c;
    }
  }
  return i;
}

void ftphelp(char * prompt){
  if(prompt == NULL){
    printf("Usage:\nget [FILENAME]\nput [FILENAME]\npwd\ndir\nls\ncd [PATH]\n?\nquit\n");
    fflush(stdout);
    return;
  }
  if(strncmp(prompt, "get", 3) == 0){
    printf("Usage:\nget [Remote-path-to-file]\n");
    fflush(stdout);
  } else if(strncmp(prompt, "put", 3) == 0){
    printf("Usage:\nput [Local-path-to-file]\n");
    fflush(stdout);
  } else if(strncmp(prompt, "pwd", 3) == 0){
    printf("Usage:\npwd\n");
    fflush(stdout);
  } else if(strncmp(prompt, "dir", 3) == 0){
    printf("Usage:\ndir\n");
    fflush(stdout);
  } else if(strncmp(prompt, "cd", 2) == 0){
    printf("Usage:\ncd [Remote-path-to-folder]\n");
    fflush(stdout);
  } else if(strncmp(prompt, "?", 1) == 0){
    printf("Usage:\n? [Command-name]\n");
    fflush(stdout);
  } else if(strncmp(prompt, "quit", 4) == 0){
    printf("Usage:\nquit\n");
    fflush(stdout);
  } else{
    printf("Usage:\nget [FILENAME]\nput [FILENAME]\npwd\ndir\nls\ncd [PATH]\n?\nquit\n");
    fflush(stdout);
  } 
  return;
}

int port(Session * sesh, int * port, FILE * stream, int download){
    int datafd, n, cfd;
    time_t t;
    int BASE = 1024;
    int MAX = 49151 - BASE;
    int BUFFER_SIZE = 100;
    char temp[BUFFER_SIZE + 1];
    char buffer[BUFFER_SIZE + 1];
    socklen_t sender_length;
    struct sockaddr_in snd_addr;

    srand((unsigned) time(&t));
    char * ip;
    if(port == NULL || *port < 0){
        *port = ((int) rand() % MAX) + BASE;
    }
    //fprintf(stderr, "Will try to connect to port %d\n", *port);
    //Create Socket
    datafd = socket(AF_INET, SOCK_STREAM, 0); // Use TCP connection, Not UDP
    if (datafd < 0) 
      perror("ERROR opening socket");
    int optval = 1;
    if(setsockopt(datafd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) 
      perror("setsockopt()");


    //initialize receiver address
    bzero((char *) &sesh->c_data, sizeof(sesh->c_data)); // initialize receiver address
    sesh->c_data.sin_family = AF_INET;
    sesh->c_data.sin_addr.s_addr = INADDR_ANY;
    sesh->c_data.sin_port = htons(*port);
      

    //bind socket to address
    while(bind(datafd, (struct sockaddr *) &sesh->c_data,
            sizeof(sesh->c_data)) < 0) {
        //printf("Blocked\n");
        *port = ((int) rand() % MAX) + BASE;
        sesh->c_data.sin_port = htons(*port);
    }
    fprintf(stderr, "Connected to port %d\n", *port);

    // int a = *port & 0xFF;
    // int b = (*port >> 8) & 0xFF;
    sprintf(temp, "%d,%d\r\n", (*port >> 8) & 0xFF, *port & 0xFF);
    if((ip = getIpAddress()) == NULL) return -1;
    sprintf(buffer, "PORT %s,%s", ip, temp);
    //fprintf(stderr, "Sending Message: (%s)", buffer);
    send(sesh->controlfd, buffer, strlen(buffer), MSG_CONFIRM);
    fflush(stdout);
    bzero(buffer, BUFFER_SIZE);

    listen(datafd,5); // 5 maximum queued request
    int pid = fork();
     if (pid < 0){
        perror("ERROR on fork");
        exit(-1);
     }
     else if (pid == 0)  { // in child
         printf("Child listening on port %d\n", *port);
         sender_length = sizeof(snd_addr);
         cfd = accept(datafd, (struct sockaddr *) &snd_addr, &sender_length);
        if (cfd < 0) {
          perror("ERROR on accept");
          exit(-1);
        } 
        printf("Child connection accepted on process %d\n", cfd);
        close(datafd);
        sesh->datafd = cfd;
        int overlap = 0;
        while(download){ // If child should be downloading the file
          bzero(buffer, BUFFER_SIZE);
          n = recv(cfd,buffer, BUFFER_SIZE, 0);
          if (n < 0){
           perror("ERROR reading from socket");
           exit(-1);
          }
          else if(n != 0){
              n = removeCharReturns(buffer, n, &overlap);
              fwrite(buffer, 1, n, stream);
              fflush(stream);
          } else {
              if(overlap)
                fwrite("\r", 1, 1, stream);
              if(stream != stdout) fclose(stream);
              exit(0);
          }
        } while(!download){ // if child should be uploading the file
          bzero(buffer, BUFFER_SIZE); 
          n = asciinpRead(buffer, BUFFER_SIZE, stream);
          if(n <= 0){
            fclose(stream);
            exit(0);
          } else if(n != 0){
              send(cfd, buffer, n, MSG_CONFIRM);
          } 
        }
        exit(0);
     } else {
      sesh->childpid = pid;
      printf("In Parent\n");
      fflush(stdout);
      //Receive PORT success command
      int overlap = 0;
      n = recv(sesh->controlfd, buffer, BUFFER_SIZE, 0);
      n = removeCharReturns(buffer, n, &overlap);
      fwrite(buffer, 1, n, stdout);
      fflush(stdout);
      if(strncmp(buffer, "2", 1) != 0){
        fprintf(stderr, "Failure to port\n");
        sesh->childpid = -1;
        close(pid);
        return -1;
      }
      return pid;
    }
    return pid;    
}



int handleCommand(enum cmdType cmd, Session * sesh, Tokens toks, size_t * numtokens){
    int portno = -1;
    size_t BUFFER_SIZE = 200;
    char buffer[BUFFER_SIZE];
    fflush(stdout);
    int pid, n;
    FILE * output;
    //int * datafd = &sesh->datafd;
    //int control = sesh->controlfd;
    //struct sockaddr_in  * snd_addr_data = &sesh->c_data;

    switch(cmd){
        case GET:
            if(*numtokens != 2){
              fprintf(stderr, "501 Syntax Error\n");
              return -1;
            } else{
                printf("Opening file (%s) for writing\n", toks[1]);
                output = fopen(toks[1], "w"); 
                if(output == NULL){
                  perror("fopen()");
                  return -1;
                }
            }
            if(sesh->state != LOGGEDIN){
                fprintf(stderr, "Not connected.\n");
            }             //First set up data connection
            if (sesh->datafd <= 0){ //No connection setup
                pid = port(sesh, &portno, output, 1);
            } 
            if(pid < 0){
                fprintf(stderr, "Error Porting\n");
                return -1;
            }else if (pid){ // in parent

                bzero(buffer, BUFFER_SIZE);                
                sprintf(buffer, "RETR %s\r\n", toks[1]);
                send(sesh->controlfd, buffer, strlen(buffer), MSG_CONFIRM);
                int i = 0; // in case server crashes?
                int overlap = 0;
                while(i++ < 20){
                  bzero(buffer, BUFFER_SIZE);
                  n = recv(sesh->controlfd, buffer, BUFFER_SIZE, 0);
                  if (n < 0){
                   perror("ERROR reading from socket");
                   exit(-1);
                  }
                  else if(n != 0){
                      n = removeCharReturns(buffer, n, &overlap);
                      fwrite(buffer, 1, n, stdout);
                      fflush(stdout);
                      if(strncmp(buffer, "2", 1) == 0){
                        close(pid);
                        fclose(output);
                        sesh->childpid = -1;
                        sesh->datafd = -1;
                        return 0;
                      }else if(strncmp(buffer, "1", 1) != 0){ //is 3,4,5
                        close(pid);
                        fclose(output);
                        sesh->childpid = -1;
                        sesh->datafd = -1;
                        return -1;
                      }
                  } else {
                      sesh->datafd = -1;
                      fclose(output);
                      perror("Timeout");
                      return -1;
                  }

                }
                return 0;
            }else{ // In child
                fclose(output);
                fprintf(stderr, "Child shouldn't return here\n");
                exit(0);
            }

        case PUT:
            if(*numtokens != 2){
              fprintf(stderr, "501 Syntax Error\n");
              return -1;
            } else{
                printf("Opening file (%s) for writing\n", toks[1]);
                output = fopen(toks[1], "r"); 
                if(output == NULL){
                  perror("fopen()");
                  return -1;
                }
            }
            if(sesh->state != LOGGEDIN){
                fprintf(stderr, "Not connected.\n");
            }             //First set up data connection
            if (sesh->datafd <= 0){ //No connection setup
                pid = port(sesh, &portno, output, 0);
            } 
            if(pid < 0){
                fprintf(stderr, "Error Porting\n");
                return -1;
            }else if (pid){ // in parent

                bzero(buffer, BUFFER_SIZE);                
                sprintf(buffer, "STOR %s\r\n", toks[1]);
                send(sesh->controlfd, buffer, strlen(buffer), MSG_CONFIRM);
                int i = 0; // in case server crashes?
                int overlap = 0;
                while(i++ < 20){
                  bzero(buffer, BUFFER_SIZE);
                  n = recv(sesh->controlfd, buffer, BUFFER_SIZE, 0);
                  if (n < 0){
                   perror("ERROR reading from socket");
                   exit(-1);
                  }
                  else if(n != 0){
                      n = removeCharReturns(buffer, n, &overlap);
                      fwrite(buffer, 1, n, stdout);
                      fflush(stdout);
                      if(strncmp(buffer, "2", 1) == 0){
                        close(pid);
                        fclose(output);
                        sesh->childpid = -1;
                        sesh->datafd = -1;
                        return 0;
                      }else if(strncmp(buffer, "1", 1) != 0){ //is 3,4,5
                        close(pid);
                        fclose(output);
                        sesh->childpid = -1;
                        sesh->datafd = -1;
                        return -1;
                      }
                  } else {
                      sesh->datafd = -1;
                      fclose(output);
                      perror("Timeout");
                      return -1;
                  }

                }
                return 0;
            }else{ // In child
                fclose(output);
                fprintf(stderr, "Child shouldn't return here\n");
                exit(0);
            }
            break;

        case LIST:
            //First set up data connection
            if (sesh->datafd <= 0){ //No connection setup
                pid = port(sesh, &portno, stdout, 1);
            } 
            if(pid < 0){
                fprintf(stderr, "Error Porting\n");
                return -1;
            }else if (pid){ // in parent
                bzero(buffer, BUFFER_SIZE);
                if(*numtokens == 2){
                  sprintf(buffer, "LIST %s\r\n", toks[1]);
                } else{
                  sprintf(buffer, "LIST\r\n");
                }
                send(sesh->controlfd, buffer, strlen(buffer), MSG_CONFIRM);
                bzero(buffer, BUFFER_SIZE);
                int overlap = 0;
                while(1){
                  bzero(buffer, BUFFER_SIZE);
                  n = recv(sesh->controlfd, buffer, BUFFER_SIZE, 0);
                  if (n < 0){
                   perror("ERROR reading from socket");
                   exit(-1);
                  }
                  else if(n != 0){
                      n = removeCharReturns(buffer, n, &overlap);
                      fwrite(buffer, 1, n, stdout);
                      fflush(stdout);
                      if(strncmp(buffer, "2", 1) == 0){
                        close(pid);
                        sesh->childpid = -1;
                        sesh->datafd = -1;
                        return 0;
                      }else if(strncmp(buffer, "1", 1) != 0){ //is 3,4,5
                        close(pid);
                        sesh->childpid = -1;
                        sesh->datafd = -1;
                        return -1;
                      }
                  } else {
                      sesh->datafd = -1;
                      perror("Timeout");
                      return -1;
                  }

                }
                return 0;
            } else{ // In child
               // parsenprint(control, *datafd);
                fprintf(stderr, "Child shouldn't return here\n");
                exit(0);
            }

        case CD:
            bzero(buffer, BUFFER_SIZE);
            if(*numtokens > 1)
                sprintf(buffer, "CWD %s\r\n", toks[1]);
            else
                sprintf(buffer, "CWD\r\n");
            
            send(sesh->controlfd, buffer, strlen(buffer), MSG_CONFIRM);
            
            bzero(buffer, BUFFER_SIZE);
            recv(sesh->controlfd, buffer, BUFFER_SIZE, 0);
            printf("%s", buffer);
            if(buffer[0] == 2){
                return 0;
            } else{
                return -1;
            }
            break;
        case PWD:
            bzero(buffer, BUFFER_SIZE);                
            send(sesh->controlfd, "PWD\r\n", 5, MSG_CONFIRM);
            int i = 0; // in case server crashes?
            int overlap = 0;
            while(i++ < 20){
              bzero(buffer, BUFFER_SIZE);
              n = recv(sesh->controlfd, buffer, BUFFER_SIZE, 0);
              if (n < 0){
               perror("ERROR reading from socket");
               exit(-1);
              }
              else if(n != 0){
                  n = removeCharReturns(buffer, n, &overlap);
                  fwrite(buffer, 1, n, stdout);
                  fflush(stdout);
                  if(strncmp(buffer, "2", 1) == 0){
                    return 0;
                  }else if(strncmp(buffer, "1", 1) != 0){ //is 3,4,5
                    return -1;
                  }
              } else {                
                  perror("Timeout");
                  return -1;
              }

            }
            break;

        case HELP:
            if(*numtokens == 2){
              ftphelp(toks[1]);
            } else{
              ftphelp(NULL);
            }
            return 0;

        case QUIT:
            send(sesh->controlfd, "QUIT\r\n", 5, MSG_CONFIRM);
            close(sesh->controlfd);
            exit(0);
        default:
            return -1;
    }
    return 0;
}

int getCommand(char * buffer, size_t size, Tokens * toks, size_t * numtokens)
{
    //ssize_t rt;
    ssize_t n = size;
    n = getline(&buffer, &size, stdin);
    if(n < 0){ 
        perror("getline()");
        return INVALID;
    }
    // fprintf(stderr, "Command: (%s)\n", buffer);
    *toks = strsplit(buffer, " \n", numtokens);

    return commandType(*toks[0]);  
}

enum cmdType commandType(char * in){
    // fprintf(stderr, "In : (%s)", in);
    if(strncmp(in, "get", 3) == 0){
        return GET;
    } else if (strncmp(in , "put", 3) == 0){
        return PUT;
    } else if (strncmp(in , "pwd", 3) == 0){
        return PWD;
    } else if (strncmp(in, "ls", 2) == 0){
        return LIST;
    }else if (strncmp(in , "dir", 3) == 0){
        return LIST;
    } else if (strncmp(in , "cd", 2) == 0){
        fprintf(stderr, "CD received\n");
        fflush(stderr);
        return CD;
    } else if (strncmp(in , "?", 1) == 0){
        return HELP;
    } else if (strncmp(in , "quit", 4) == 0){
        return QUIT;
    }else if (strncmp(in , "exit", 4) == 0){
        return QUIT;
    } else if (strncmp(in, "help", 4) == 0){
        return HELP;
    } else{
        return INVALID;
    }
}

char * getIpAddress()
{
struct ifaddrs *ifaddr, *ifa;
int family, s, n;
char host[NI_MAXHOST];
char addr[50];
char * ret;

if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    return NULL;
}

/* Walk through linked list, maintaining head pointer so we
can free list later */

for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL)
       continue;

    family = ifa->ifa_addr->sa_family;

    /* For an AF_INET* interface address, display the address */

    if (family == AF_INET /*|| family == AF_INET6*/) {
       s = getnameinfo(ifa->ifa_addr,
               (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                     sizeof(struct sockaddr_in6),
               host, NI_MAXHOST,
               NULL, 0, NI_NUMERICHOST);
       if (s != 0) {
           printf("getnameinfo() failed: %s\n", gai_strerror(s));
           return NULL;
       }
       if(strncmp("127.0.0.1", host, 9) == 0){ // don't give it the loopback
              sprintf(addr, "%s", host);
              break;
       }
    }/* else if (family == AF_PACKET && ifa->ifa_data != NULL) {
       struct rtnl_link_stats *stats = ifa->ifa_data;

       printf("\t\ttx_packets = %10u; rx_packets = %10u\n"
              "\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
              stats->tx_packets, stats->rx_packets,
              stats->tx_bytes, stats->rx_bytes);
    }*/
}

    freeifaddrs(ifaddr);
    n = strlen(addr);
    for(int i = 0; i < n; i++){
        if(addr[i] == '.')
            addr[i] = ',';
    }
    ret = strdup(addr);
    return ret;
}


int downloadFile(int control, int port, char * filename)
{
  int cfd, datafd, portno, pid;
  socklen_t sender_length;
  
  
  char temp[50];
  char buffer[100];
  char * ip;
  struct sockaddr_in rcv_addr, snd_addr;
  
  //Create Socket
  datafd = socket(AF_INET, SOCK_STREAM, 0/*SOCK_NONBLOCK*/); // Use TCP connection, Not UDP
  if (datafd < 0) 
    perror("ERROR opening socket");
  int optval = 1;
  if(setsockopt(datafd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) 
    perror("setsockopt()");


  //initialize receiver address
  bzero((char *) &rcv_addr, sizeof(rcv_addr)); // initialize receiver address
  portno = port;
  rcv_addr.sin_family = AF_INET;
  rcv_addr.sin_addr.s_addr = INADDR_ANY;
  rcv_addr.sin_port = htons(portno);
  
  //bind socket to address
  if (bind(datafd, (struct sockaddr *) &rcv_addr,
          sizeof(rcv_addr)) < 0) 
          perror("bind(): must have authorized access.");

  int a = port & 0xFF;
  int b = (port >> 8) & 0xFF;
  sprintf(temp, "%d,%d\r\n", a, b);
  if((ip = getIpAddress()) == NULL) return -1;
  sprintf(buffer, "PORT %s,%s", ip, temp);
  send(control, buffer, strlen(buffer), MSG_CONFIRM);
  listen(datafd,5); // 5 maximum queued requests
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
         dataConnect(cfd, &snd_addr, filename);
         exit(0);
     } else {
      printf("In Parent\n");
      fflush(stdout);
     //close(datafd); // in parent
      waitpid(pid, NULL, 0);
      break;
    }
  } /* end of while */
  close(datafd);
  return 0; /* we never get here */
}

void dataConnect (int sock,  struct sockaddr_in * client_addr, char * filename)
{
  int maxfd;
  ssize_t n;
  size_t BUFFER_SIZE = 1024;
  char buffer[BUFFER_SIZE];
  bzero(buffer,256);
  // enum State state = NEEDUSER; // initialize to need user
  fd_set rfds; // File descriptors to be read
  struct timeval tv; // Timer for read value
  int retval; // Return value for the timeval
  if(client_addr == NULL) perror("useless thing");
  FILE * fp = fopen(filename, "w"); // Make this dynamic..
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
            fwrite(buffer, 1, n, fp);
        } 
    } else {
      fclose(fp);
      perror("Timeout");
    }
  }

  printf("Out of loop\n");
}

int connect_to_host(char * host, Session * sesh, fd_set * rfds){
    int sockfd;
    char buffer[200];
    const int _PORT_L = 21;
    struct hostent * retServer;
    struct sockaddr_in * serv_addr = &sesh->c_control;

    //Create Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");


    FD_ZERO(rfds);
    FD_SET(sockfd, rfds);

    //Get the IP address
    retServer = gethostbyname(host);
    if (retServer == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }


    bzero((char *) serv_addr, sizeof(*serv_addr));
    serv_addr->sin_family = AF_INET;
    bcopy((char *)retServer->h_addr, 
         (char *)&(serv_addr->sin_addr.s_addr),
         retServer->h_length);
    serv_addr->sin_port = htons(_PORT_L);

    //TODO: if client wants to specify port number, need way to BIND...
    if (connect(sockfd,(struct sockaddr *) serv_addr,sizeof(*serv_addr)) < 0){
        fprintf(stderr, "Error connecting.\n");
        return -1;
    } 

    int i =  0;
    while(i++ < 10){
        bzero(buffer, sizeof(buffer));
        recv(sockfd,buffer,255, 0);
        printf("%s", buffer);
        if(strncmp(buffer, "220", 3) == 0){
            printf("Connected to %s\n", host);
            sesh->controlfd = sockfd;
            return sockfd;
        } else if(strncmp(buffer, "1", 1) != 0){
            printf("Unable to establish connection.\n");
            return -1;
        }
        
    }
    //Or if return 421
    return -2;
}


int waitForMessage(int sockfd, char * buffer, const int bufferSize, fd_set * rfds){
    int retval, n;
    int maxfd = sockfd + 1;
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    bzero(buffer, sizeof(char) * bufferSize);
    retval = select(maxfd, rfds, NULL, NULL, &tv);
    if(retval == -1) error("select()");
    else if (retval)
    {
        n = recv(sockfd,buffer,255, 0);
        #ifdef DEBUG
        if(n > 2){
            buffer[n - 2] = '\0';
        }
        #endif
        if (n < 0) 
             error("ERROR reading from socket");
        else if(n != 0){
            int overlap = 0;
            n = removeCharReturns(buffer, n, &overlap);
            fwrite(buffer, 1, n, stdout);
            fflush(stdout);
            //printf("%d: %s\n",n, buffer);
            return 0;
        } 
        else{
            fprintf(stderr, "Why did n = %d?\n", n);
            return -1;
        }
    }
    else
    {
        printf("Timeout\n");
        return -1;
    }
    return -1; // Shouldn't get here...
}


int login(int fd, Session * sesh, fd_set * rfds){
    char buffer[256];
    char formatted[300];
    enum State * state = &sesh->state;
    enum State oldState = NEEDUSER;
    int n, retval;
    int maxfd = fd + 1;
    struct timeval tv;
    //Input User
    int i = 0;
    while(i++ < 10){
        if (*state == NEEDUSER)
        {
            bzero(buffer, sizeof(buffer));
            bzero(formatted, sizeof(formatted));
            printf("Name: ");
            fgets(buffer, sizeof(buffer) - 1,stdin);
            sprintf(formatted, "USER %s\r\n", buffer);
            n = send(fd,formatted,strlen(formatted), MSG_CONFIRM);
            if (n < 0) 
                 error("ERROR writing to socket");
            bzero(buffer,256);
            oldState = *state;
            *state = WAIT;
        }
        else if (*state == NEEDPASS)
        {
            bzero(buffer, 256);
            printf("Password: ");
            fgets(buffer,255,stdin);
            bzero(formatted, 300);
            sprintf(formatted, "PASS %s\r\n", buffer);
            n = send(fd, formatted, strlen(formatted), MSG_CONFIRM);
            oldState = *state;
            *state = WAIT;
        }
        else if(*state == WAIT)
        {
            tv.tv_sec = 10;
            tv.tv_usec = 0;
            retval = select(maxfd, rfds, NULL, NULL, &tv);
            if(retval == -1)error("select()");
            else if (retval)
            {
                n = recv(fd,buffer,255, 0);
                if (n < 0) error("ERROR reading from socket");
                else if(n != 0){
                    printf("%d: %s",n, buffer);
                    if(strncmp(buffer, "QUIT", 4) == 0){
                        printf("QUIT received\n");
                        close(fd);
                        return 1;
                    } else if (oldState == NEEDUSER && strncmp(buffer, "331", 3) == 0){
                       printf("User OK, Input Password.\n");
                       *state = NEEDPASS;
                    } else if (oldState == NEEDPASS && strncmp(buffer, "230", 3) == 0){
                        printf("Login success. returning.\n");
                        *state = LOGGEDIN;
                        return 0;
                    }
                    else{
                        printf("unaccounted for. Exiting\n");
                        close(fd);
                        return -1;
                    }
                } 
            }
            else
            {
                printf("Timeout received. Exiting.\n");
                close(fd);
                return -1;
            }

        }
        else
        {
            printf("This makes 0 sense.\n");
        }
        
            
    }
    return 0;
}

//Adapted from https://www.quora.com/How-do-you-write-a-C-program-to-split-a-string-by-a-delimiter
Tokens strsplit(const char* str, const char* delim, size_t* numtokens) {
    // copy the original string so that we don't overwrite parts of it
    // (don't do this if you don't need to keep the old line,
    // as this is less efficient)
    char *s = strdup(str);
    // these three variables are part of a very common idiom to
    // implement a dynamically-growing array
    size_t tokens_alloc = 1;
    size_t tokens_used = 0;
    char ** tokens = calloc(tokens_alloc, sizeof(char*));
    char *token, *strtok_ctx;
    for (token = strtok_r(s, delim, &strtok_ctx);
            token != NULL;
            token = strtok_r(NULL, delim, &strtok_ctx)) {
        // check if we need to allocate more space for tokens
        if (tokens_used == tokens_alloc) {
            tokens_alloc *= 2;
            tokens = realloc(tokens, tokens_alloc * sizeof(char*));
        }
        tokens[tokens_used++] = strdup(token);
    }
    // cleanup
    if (tokens_used == 0) {
        free(tokens);
        tokens = NULL;
    } else {
        tokens = realloc(tokens, tokens_used * sizeof(char*));
    }
    *numtokens = tokens_used;
    free(s);

    return tokens;
}

int freeTokens(Tokens in, size_t numtokens){
  for (size_t i = 0; i < numtokens; i++) {
            free(in[i]);
  }
  if (in != NULL)
      free(in);
    return 1;
}

int removeCharReturns(char * b, int size, int * overlap){
  int i = 0,  j = 0, k;
  int n = size;
  //printf("%d bytes before:\n(%s)\n", size, b);
  fflush(stdout);
  if(*overlap == 1 && b[0] != '\n'){
    printf("OVERLAP! WEIRD\n");
    for(int v = size; v > 0; v--){
      b[v] = b[v - 1];
    }
    b[0] = '\r';
    n++;
  } 
  *overlap = 0;

  for (; i < n && j < n; i++){
      if(j < (size - 1) && b[j] == '\r' && b[j+1] == '\n'){
        j++;
      } else if(j == n -1 && b[j] == '\r'){
        printf("There is overlap\n");
        fflush(stdout);
        *overlap = 1;
        break;
      }
      b[i] = b[j];
      j++;
  }
  for(k = i; k < n; k++){
    b[k] = '\0';
  }
  //printf("%d bytes after:\n(%s)\n", i, b);
  fflush(stdout);
  return i;
}