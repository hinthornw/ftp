#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <features.h>
#include <time.h>
#include "sservices.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}


char * toUpperCase(char * in){
  char * copy = strdup(in);
  int i, n, c;
  n = strlen(copy);
  for(i = 0; i < n; i++){
    c = copy[i];  
    if (isalpha(c) && islower(c)){
      copy[i] = toupper(c);
    }
  }

  return copy;
}
// Change the working directory
int cd(char * c){
  int r = chdir(c);
  if(r < 0) perror("chdir()");
  return r;
}

void handler(int n){
  fprintf(stderr, "Signal %d\n", n);
}

/*int changeType(Session * sesh, Tokens tok){



  if(strcmp(tok[2], ""))

}*/

int changeStruc(Session * sesh, Tokens tok){
  if(tok == NULL || sesh == NULL){
    fprintf(stderr, "changeStruc(): input null\n");
    return -1;
  }
  if (strcmp(tok[1], "F") == 0){
    sesh->structure = sFILE;
    return 0;

  } else if (strcmp(tok[1], "R") == 0){
    sesh->structure = sREC;
    return 0;
    
  } else if (strcmp(tok[1], "P") == 0){
    return 1;    
  } else{
    return -1;
  }
}


int changePort(Session * sesh, Tokens tok)
{
  if(sesh == NULL || tok == NULL){
    return -1;
  }
  int ipa, ipb, ipc, ipd;
  int porta, portb;
  unsigned char ip[4], port[2];
  uint32_t * ipVal;
  uint16_t * portVal;

  ipa = atoi(tok[1]);
  ipb = atoi(tok[2]);
  ipc = atoi(tok[3]);
  ipd = atoi(tok[4]);
  porta = atoi(tok[5]);
  portb = atoi(tok[6]);

  ip[0] = ipa & 0xFF;
  ip[1] = ipb & 0xFF;
  ip[2] = ipc & 0xFF;
  ip[3] = ipd & 0xFF;
  port[0] = porta & 0xFF;
  port[1] = portb & 0xFF;
  
  ipVal = (uint32_t *) ip;
  portVal = (uint16_t * ) port;
  sesh->c_data.sin_addr.s_addr = *ipVal; // REAALLY sketchy...
  sesh->c_data.sin_port = *portVal;

  // #ifdef DEBUG
  // fprintf(stderr, "(%d.%d.%d.%d), (%d,%d)\n%X\t%X\n", ipa, ipb, ipc, ipd, porta, portb,
  //  (uint32_t) port[0], (uint16_t) port[1]);
  // fprintf(stderr, "IMPORTANT:IP is %s\n", inet_ntoa(sesh->c_data.sin_addr));
  // fprintf(stderr, "Port is: %d\n", ntohs(sesh->c_data.sin_port));
  // fflush(stderr);
  // printf("LIST OF ADDRESSES\n%p\t%p\n%hu\t%hu\n",
  //   &sesh->c_control.sin_port, &sesh->c_data.sin_port,
  //   ntohs(sesh->c_control.sin_port), ntohs(sesh->c_data.sin_port));
  // #endif
  return 0;
}

int asciinpWrite(Session * sesh, char * buffer,const int BUFFER_SIZE, FILE * fp){
  int c, /*d,*/ i = 0, j = 0; 
  if(sesh->structure == sREC){ // if rec, must indicate EOF explicitly
        fprintf(stderr, "File transferred is record type. Will have errors.\n");
  }

  for(i = 0; i < BUFFER_SIZE; i++){
    c = buffer[i];
    if(c == '\r'){
      if(i < BUFFER_SIZE -1){
        if(buffer[i+1] == '\n'){
          c = '\n';
          i++;
        } 
      }else{
        fprintf(stderr, "Carriage return at end of buffer.\n");
        //return j;
      }
    } /*else if (i == 0 && c == '\n'){ // if first char in buffer is \n, preceding char COULD be \r
      d = fgetc(fp);
      if (d != '\r')
        fputc(d, fp);
    }*/
    j++;
    fputc(c, fp);
  }

  return j;
}
int asciinpRead(Session * sesh, char * buffer, const int BUFFER_SIZE, FILE * stream){
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
      if(sesh->structure == sREC){ // if rec, must indicate EOF explicitly
        buffer[i++] = 0xFF; // i.e. '\0'
        buffer[i] = 0x01;   // i.e. EOF
      }
      //i--;
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

int storeFile(Session * sesh, char * filename, int * sDataSocket, int control)
{

  //Open file name with write access
  //continue to insert data until is done.
  //convert /r/n to /n
  //Yeah....
  // STOR
  //     125, 150
  //        (110)
  //        226, 250
  //        425, 426, 451, 551, 552
  //     532, 450, 452, 553
  //     500, 501, 421, 530
    int  maxfd;
    int ctrlReturn;
    enum ftpCommands cmd;
    ssize_t  rt, retval;
    size_t  n;  //TODO: Simplify
    fd_set rfds; // File descriptors to be read
    FILE * fp;
    struct timeval tv; // Timer for read value
    const int BUFFER_SIZE = 1024; // Kind of arbitrary...
    char buffer[BUFFER_SIZE];
    Tokens tokens;
    size_t numtokens;
    bzero(buffer, sizeof(buffer));

    if(filename == NULL){
      fprintf(stderr, "File not legal.\n");
      send(control, "501 illegal filename\r\n", 22, MSG_CONFIRM);
      return -1;
    }
    #ifdef DEBUG
    fprintf(stderr, "Receiving file (%s) from user %s on socket %d. Control %d.\n", filename, sesh->usr, *sDataSocket, control);
    #endif
    //Begin transfer by opening file
    fp = fopen(filename, "w"); 

    if(!fp) {
      send(control, "452 illegal filename\r\n", 22, MSG_CONFIRM);
      perror("Error opening file"); 
      fclose(fp);
      return -1;
    } else {
      if(*sDataSocket < 0){
        //send(control, "257 %s created\r\n",14 + strlen(filename), MSG_CONFIRM);
        send(control, "150 File status okay; about to open data connecton\r\n",52, MSG_CONFIRM);
      }
      else
        send(control, "125 Data connection already open; transfer starting\r\n",53, MSG_CONFIRM);
    }
    
    //Initialize the to read FD.
    //TODO: Figure out how to also stop if get an out of band message...
    FD_ZERO(&rfds);
    FD_SET(*sDataSocket, &rfds);
    FD_SET(control, &rfds);
    maxfd = *sDataSocket+1;
    if (maxfd <= control){
      maxfd = control + 1;
    }
    int i = 0;
    fprintf(stderr, "Waiting for data.\n");
    while(i++ < 1000000)
    {
      
      bzero(buffer,BUFFER_SIZE);

      // rt = recv(*sDataSocket,buffer, BUFFER_SIZE, 0);
      // fprintf(stderr, "Data Received: (%s)\n", buffer);
      // bzero(buffer,BUFFER_SIZE);
      

      //TESTING^^^///
      tv.tv_sec = 20;
      tv.tv_usec = 0;
      retval = select(maxfd, &rfds, NULL, NULL, &tv);
      if(retval < 0){fclose(fp); perror("select()"); return -1;}
      else if (retval) {
          fprintf(stderr, "\nSelect() triggered\n");
          if(FD_ISSET(*sDataSocket, &rfds))
         { 
            fprintf(stderr, "Data found \n");
            rt = recv(*sDataSocket,buffer, BUFFER_SIZE, 0);
            //fprintf(stderr, "Data: (%s)\n", buffer);
            if (rt < 0) perror("ERROR reading from socket");
            else if (rt == 0){
              send(control, "226 CLosing data conneciton. File transfer completed\r\n", 54, MSG_CONFIRM);
              fprintf(stderr, "Message end\n");
              fclose(fp);
              close(*sDataSocket);
              return 0;
            }
          } if (FD_ISSET(control, &rfds)){
            //fprintf(stderr, "Control found \n");
            rt = recv(control,buffer, BUFFER_SIZE, 0);
            //fprintf(stderr, "Control: (%s)\n", buffer);
            send(control, "503 Bad sequence of commands.\r\n", 30, MSG_CONFIRM);
            tokens = strsplit(buffer, ", \t\r\n", &numtokens);
            cmd = checkMessage(control, tokens);
            ctrlReturn = handleMessage(sesh, cmd, tokens, numtokens, control);
            if(ctrlReturn < 0){
              fprintf(stderr,"Undoable\n");
            }
            continue;

          }

      } else {
        send(control, "226 CLosing data conneciton. File transfer completed\r\n", 54, MSG_CONFIRM);
        // send(control, "426 Connection closed. transfer aborted\r\n",41, MSG_CONFIRM);
        fclose(fp);
        close(*sDataSocket);
        fprintf(stderr, "Timeout\n");
        return 0;
      }

      /***   HMMM How to input  ***/
      if(sesh->type == ASCIINP){
        n = asciinpWrite(sesh, buffer, BUFFER_SIZE, fp); //Modify to write
      } else if (sesh->type == BINARY){
        n = fread(buffer, 1, BUFFER_SIZE, fp); // Modify to write
      } else{
        fprintf(stderr, "File not ascii??\n");
      }

      /*** HMMM How to input ***/
     if(n <= 0){
        #ifdef DEBUG
        printf("EOF reached after %d transfers. CLosing stream.\n", i);
        #endif
        send(control, "226 CLosing data conneciton. File transfer completed\r\n", 54, MSG_CONFIRM);
        fclose(fp);
        close(*sDataSocket);
        return 0;
      }    

    }
  return 0;
}

//Only call AFTER connection is established
int transferFile(Session * sesh, char * filename, int sDataSocket, int control)
{
    int  maxfd;
    ssize_t  rt, retval;
    size_t  n;  //TODO: Simplify
    fd_set wfds; // File descriptors to be read
    FILE * fp;
    struct timeval tv; // Timer for read value
    const int BUFFER_SIZE = 1024; // Kind of arbitrary...
    char buffer[BUFFER_SIZE];
    bzero(buffer, sizeof(buffer));

    if(filename == NULL){
      fprintf(stderr, "File not legal.\n");
      send(control, "550 File not found\r\n", 20, MSG_CONFIRM);
      return -1;
    }

    #ifdef DEBUG
    printf("In transferFile\n");
    #endif
    //Begin transfer by opening file
    fp = fopen(filename, "r"); 
    if(fp == NULL) {
      send(control, "550 File not found\r\n", 20, MSG_CONFIRM);
      perror("Error opening file"); 
      fclose(fp);
      return -1;
    } else{
      if(sDataSocket < 0)
        send(control, "150 File status okay; about to open data connecton\r\n",52, MSG_CONFIRM);
      else
        send(control, "125 Data connection already open; transfer starting\r\n",53, MSG_CONFIRM);
    }
    
    //Initialize the to read FD.
    //TODO: Figure out how to also stop if get an out of band message...
    FD_ZERO(&wfds);
    FD_SET(sDataSocket, &wfds);
    maxfd = sDataSocket+1;
    int i = 0;
    while(i++ < 100000){
    bzero(buffer,BUFFER_SIZE);
    if(sesh->type == ASCIINP){
      n = asciinpRead(sesh, buffer, BUFFER_SIZE, fp);
    } else if (sesh->type == BINARY){
      n = fread(buffer, 1, BUFFER_SIZE, fp);
    } else{
      fprintf(stderr, "File not ascii??\n");
    }
    if(n <= 0){
      #ifdef DEBUG
      printf("EOF reached after %d transfers. CLosing stream.\n", i);
      #endif
      send(control, "226 CLosing data conneciton. File transfer completed\r\n", 54, MSG_CONFIRM);
      fclose(fp);
      close(sDataSocket);
      return 0;
    }
    tv.tv_sec = 20;
    tv.tv_usec = 0;
    retval = select(maxfd, NULL, &wfds, NULL, &tv);
      if(retval < 0){fclose(fp); perror("select()");}
      else if (retval) {
          rt = send(sDataSocket, buffer, n, MSG_CONFIRM);
          //NEED TO FIND A WAY TO FIND EOF
          if (rt < 0) perror("ERROR reading from socket");
          
      } else {
        send(control, "426 Connection closed. transfer aborted\r\n",41, MSG_CONFIRM);
        fclose(fp);
        perror("Timeout");
        return 1;
      }

    }
    return 0;
}

int connect_to_client(Session * sesh) 
{
    int  sockfd;
    const int _PORT_LD = 20;
    if (sesh == NULL) perror("Sesh is null\n");
    struct sockaddr_in * client_addr = &sesh->c_data; // Mask the client data
    #ifdef DEBUG
    printf("In connect_to_client\n");
    #endif
    /*** Create and bind server socket to Port L - 1 (ftp port 20) ***/
    struct sockaddr_in data_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        perror("ERROR opening socket");
    int optval = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) 
      perror("setsockopt()");
    //FTP server MUST send data from port 20

    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = INADDR_ANY;
    data_addr.sin_port = htons(_PORT_LD);
    while(bind(sockfd, (struct sockaddr *) &data_addr,
          sizeof(data_addr)) < 0) {
      sleep(1);
    }
    /*** Server socket has been created and bound **/
       
    
    /** Connect to client's data port (default Port U i.e. same as control)**/
    if (connect(sockfd,(struct sockaddr *) client_addr,sizeof(*client_addr)) < 0){
        fprintf(stderr, "Error connecting.\n");
        return -1;
    } 
    #ifdef DEBUG
    printf("Connected to %d on port %d\n", ntohs(client_addr->sin_addr.s_addr), ntohs(client_addr->sin_port));
    #endif
        // error("ERROR connecting");

    return sockfd; // Return the socket that is currently connected to client
}



int login(Session * sesh, int sock, char * buffer){
  int n, maxfd;
  enum servState state = NEEDUSER; // initialize to need user
  fd_set rfds; // File descriptors to be read
  struct timeval tv; // Timer for read value
  int retval; // Return value for the timeval
  size_t numtokens;
  #ifdef DEBUG
  printf("In login\n");
  #endif
  Tokens tok;


  //Initialize the to read FD.
  FD_ZERO(&rfds);
  FD_SET(sock, &rfds);
  maxfd = sock+1;
  while(1){
    // #ifdef DEBUG
    // if(state == NEEDUSER){
    //   fprintf(stderr, "Need Username\n");
    //   fflush(stderr);
    // }else if (state == NEEDPASS){
    //   printf("Need Password\n");
    //   fflush(stderr);
    // }
    // #endif

    bzero(buffer,1024);
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    retval = select(maxfd, &rfds, NULL, NULL, &tv);
    if(retval == -1)error("select()");
    else if (retval)
    {
        n = recv(sock,buffer,1024, 0);
        if (n < 0) error("ERROR reading from socket");
        else if(n != 0){
          tok = strsplit(buffer, ", \t\r\n", &numtokens);
          printf("%lu: %s\n",numtokens, tok[0]);   
        } 
        else{continue;}
  } else {
      printf("Timeout\n");
      return -1;
  }

  if(state == NEEDUSER && strncmp(tok[0], "USER", 4) == 0){ // User signin
      #ifdef DEBUG
      fprintf(stderr, "Received Username\n");
      fflush(stderr);
      #endif
      if(state != NEEDUSER) {
        freeTokens(tok, numtokens);
        error("Error: Username already input.\n");
      }
      else{
        strncpy(sesh->usr, tok[1], sizeof(sesh->usr));
        n = send(sock,"331 User name ok, need password\r\n",33, MSG_CONFIRM);
        if (n < 0) error("ERROR writing to socket");
        state = NEEDPASS;
        freeTokens(tok, numtokens);
      }
     }
  else if (state == NEEDPASS && strncmp(tok[0], "PASS", 4) == 0){ //password
      #ifdef DEBUG
      fprintf(stderr, "Received Password\n");
      fflush(stderr);
      #endif
      if(state!= NEEDPASS){
        error("Error: Password input out of order.\n");
      } else{
        strncpy(sesh->psw, tok[1], sizeof(sesh->psw));
        fprintf(stderr, "User: %s\nPass: %s\n", sesh->usr, sesh->psw);
        n = send(sock,"230 User logged in\r\n",20, MSG_CONFIRM);
        if (n < 0) error("ERROR writing to socket");
        freeTokens(tok, numtokens);
        return 0;
      }
  } else{
      #ifdef DEBUG
      fprintf(stderr, "Not pass or user. Exiting.\n");
      fflush(stderr);
      #endif
      freeTokens(tok, numtokens);
      return -1;
  }
  }
}

//Could be a 
int list_dir(Session * sesh, int control, int data,  char * path)
{
  int BUFFER_SIZE = 1202;
  int width = 15;
  char buffer[BUFFER_SIZE];
  DIR *dp;
  FILE * fp;
  // int isDir = 1;
  struct stat status;
  struct dirent *ep, *result;  
  int rt, i; 
  char * dirpath = "./";

  /*** Confirm data connection is open ***/
  if(data < 0){
      //send(control, "257 %s created\r\n",14 + strlen(filename), MSG_CONFIRM);
      send(control, "150 File status okay; about to open data connecton\r\n",52, MSG_CONFIRM);
    } else{
      send(control, "125 Data connection already open; transfer starting\r\n",53, MSG_CONFIRM);
  }


  if (path != NULL){
    fprintf(stderr, "Sending %s listings to %s on %d with %d control channel.\n",
        path, sesh->usr, data, control);
    dirpath = strdup(path);
  }
  else
    dirpath = "./"; 

  dp = opendir(dirpath);
  if(dp){
   int name_max = pathconf(dirpath, _PC_NAME_MAX);
   if (name_max == -1)         /* Limit not defined, or error */
       name_max = 255;         /* Take a guess */
    int len = offsetof(struct dirent, d_name) + name_max + 1;
    ep = malloc(len);
    if (dp != NULL)
    {
      i = 0;
      do{
        bzero(buffer, BUFFER_SIZE);
        while (i < BUFFER_SIZE - width - 2){
          rt = readdir_r (dp, ep, &result);
          if(rt != 0){perror("Error reading file");}
          else if (result == NULL) break;
          else if(strcmp(ep->d_name, "." ) == 0 || strcmp(ep->d_name, "..") == 0) continue;
          else {
          sprintf(&buffer[i], "%*.*s", -width, width, ep->d_name/*, ep->d_type*/);
          }
          i += width;
        }
        buffer[i++] = '\r';
        buffer[i++] = '\n';
        send(data, buffer, i, MSG_CONFIRM);
      } while (result != NULL);
      (void) closedir (dp);
      send(control, "226 Directory Contents Sent\r\n", 29 , MSG_CONFIRM);
    }
    else
      perror ("Couldn't open the directory");

    if (path != NULL) free(dirpath);
    return 0;
  }
  else{ // Could be a file
    fp = fopen(dirpath, "r");
    if(!fp){ // ILLEGAL
    free(dirpath);
    perror("Path not valid dir or file");
    return -1;
    }
    fclose(fp);
    rt = stat(dirpath, &status);
    if(rt < 0){
      perror("stat()");
      fclose(fp);
      free(dirpath);
      return -1;
    }
    char temp[30];
    bzero(temp, 30);
    bzero(buffer, BUFFER_SIZE);
    i = 0;
    buffer[i++] = S_ISDIR(status.st_mode) ? 'd' : '-';
    buffer[i++] = status.st_mode & S_IRUSR ? 'r' : '-';
    buffer[i++] = status.st_mode & S_IWUSR ? 'w' : '-';
    buffer[i++] = status.st_mode & S_IXUSR ? 'x' : '-';
    buffer[i++] = status.st_mode & S_IRGRP ? 'r' : '-';
    buffer[i++] = status.st_mode & S_IWGRP ? 'w' : '-';
    buffer[i++] = status.st_mode & S_IXGRP ? 'x' : '-';
    buffer[i++] = status.st_mode & S_IROTH ? 'r' : '-';
    buffer[i++] = status.st_mode & S_IWOTH ? 'w' : '-';
    buffer[i++] = status.st_mode & S_IXOTH ? 'x' : '-';
    buffer[i++] = ' ';
    send(data, buffer, i, MSG_CONFIRM);
    //Num Links
    sprintf(temp, "%lu", status.st_nlink);
    sprintf(&buffer[i], "%s", temp);
    i += strlen(temp);
    buffer[i++] = ' ';
    send(data, buffer, i, MSG_CONFIRM);
    //UID
    sprintf(temp, "%u", status.st_uid);
    sprintf(&buffer[i], "%s", temp);
    i += strlen(temp);
    buffer[i++] = ' ';
    //GID
    sprintf(temp, "%u", status.st_gid);
    sprintf(&buffer[i], "%s", temp);
    i += strlen(temp);
    buffer[i++] = ' ';
    //Size
    sprintf(temp, "%ld", status.st_size);
    sprintf(&buffer[i], "%s", temp);
    i += strlen(temp);
    buffer[i++] = ' ';
    //Last modification

    sprintf(temp, "%s", ctime(&status.st_mtime));
    temp[strlen(temp) - 1] = '\0';
    sprintf(&buffer[i], "%s\r\n", temp);
    i = strlen(buffer);
  
    send(data, buffer, strlen(buffer), MSG_CONFIRM);
    send(control, "226 Directory Contents Sent\r\n", 29 , MSG_CONFIRM);
    if (path != NULL) free(dirpath);
    return 0;
    //-rw-rw-r-- 1 1000 1000 11816 Fri Dec 16 22:09:06 2016  
  }
  
}


//checkCommand
//Look at first four characters,
//If not implemented or illegal -> send message saying so
//return the servCommand for the proper jawn
//

enum ftpCommands checkMessage(int sock, Tokens buffer){
  char notImplemented[26];
  char * invalid = "501 Syntax Error\r\n";
  bzero(notImplemented, sizeof(notImplemented));
  int n; // TODO Not needed...
  enum ftpCommands cmd;
  if (!buffer){
    #ifdef DEBUG
    fprintf(stderr, "Buffer is NULL\n");
    fflush(stderr);
    #endif
    return INVALID;
  }

  #ifdef DEBUG
  fprintf(stderr, "\nChecking command: (%s)\n", buffer[0]);
  fflush(stderr);
  #endif

  if(strncmp(buffer[0], "QUIT", 4) == 0){
    cmd = QUIT;
    return cmd;
  }else if(strncmp(buffer[0], "PORT", 4) == 0){
    cmd = PORT;
    return cmd;
  }else if(strncmp(buffer[0], "TYPE", 4) == 0){
    cmd = TYPE;
    return cmd;
  }else if(strncmp(buffer[0], "MODE", 4) == 0){
    cmd = MODE;
    return cmd;
  }else if(strncmp(buffer[0], "STRU", 4) == 0){
    cmd = STRU;
    return cmd;
  }else if(strncmp(buffer[0], "RETR", 4) == 0){
    cmd = RETR;
    return cmd;
  }else if(strncmp(buffer[0], "STOR", 4) == 0){
    cmd = STOR;
    return cmd;
  }else if(strncmp(buffer[0], "NOOP", 4) == 0){
    cmd = NOOP;
    return cmd;
  }else if(strncmp(buffer[0], "USER", 4) == 0){
    send(sock,"503 Bad Sequence\r\n",18, MSG_CONFIRM);
    return INVALID;
  }else if(strncmp(buffer[0], "PASS", 4) == 0){
    send(sock,"503 Bad Sequence\r\n",18, MSG_CONFIRM);
    return INVALID;
  }else if(strncmp(buffer[0], "STOU", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "STOU");
    n = send(sock, notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "APPE", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "APPE");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "ALLO", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "ALLO");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "REST", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "REST");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "RNFR", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "RNFR");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "RNTO", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "RNTO");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "ABOR", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "ABOR");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "DELE", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "DELE");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "MKD", 3) == 0){
    sprintf(notImplemented, "502 %s  not implemented\r\n", "MKD");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "RMD", 3) == 0){
    sprintf(notImplemented, "502 %s  not implemented\r\n", "RMD");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "PWD", 3) == 0){
    sprintf(notImplemented, "502 %s  not implemented\r\n", "PWD");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "CWD", 3) == 0){
    //sprintf(notImplemented, "502 %s  not implemented\r\n", "CWD");
    //n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    //if(n < 0) perror("Send err failed");
    cmd = CWD;
    return cmd;
  }else if(strncmp(buffer[0], "LIST", 4) == 0){
    //sprintf(notImplemented, "502 %s not implemented\r\n", "LIST");
    //n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    //if(n < 0) perror("Send err failed");
    cmd = LIST;
    return cmd;
  }else if(strncmp(buffer[0], "NLST", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "NLST");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "SITE", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "SITE");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "SYST", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "SYST");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "HELP", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "HELP");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "ACCT", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "ACCT");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "CDUP", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "CDUP");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "SMNT", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "SMNT");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "REIN", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "REIN");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "PASV", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "PASV");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else{
    #ifdef DEBUG
    fprintf(stderr, "\nCommand: (%s) has no match.\n", buffer[0]);
    fflush(stderr);
    #endif
    n = send(sock, invalid, sizeof(invalid), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }

}



int handleMessage(Session * sesh, enum ftpCommands in, Tokens tokens, size_t numtokens, int sock)
{
  int datafd;
  int n;
  char * pathname;
  switch(in)
  {
    case INVALID:
      return -1;              
    case QUIT:
      #ifdef DEBUG
      printf("QUIT received. Closing connections.\n");
      #endif
      close(sock);
      return -2;
    case PORT:
      if(changePort(sesh, tokens) < 0) {
        error("changePort()");
        return 0;
      }
      else{
        n = send(sock,"200 Port List Change\r\n",22, MSG_CONFIRM);
        return 1;
      }
    case TYPE:
      fprintf(stderr, "TYPE call received\n");
      //n = send(sock, "", , MSG_CONFIRM);
      return -1;
    case MODE:
      fprintf(stderr, "MODE call received\n");
      // set to stream, block, compressed
      return -1;
    case LIST:
      fprintf(stderr, "LIST call received\n");
      datafd = connect_to_client(sesh);
      if (numtokens >= 2)
        pathname = tokens[1];
      else
        pathname = NULL;
      if(list_dir(sesh,sock, datafd, pathname) < 0){
        send(sock, "450 Invalid Pathname\r\n", 22, MSG_CONFIRM);
      }else {
        close(datafd);
      }
      return 0;
    case CWD:
        fprintf(stderr, "CWD call received\n");
        if(numtokens == 2){
          pathname = tokens[1];
        } else{
          send(sock, "501 Invalid Pathname\r\n", 22, MSG_CONFIRM);
          return -1;
        }
        if (cd(pathname) < 0){
          send(sock, "501 Invalid Pathname\r\n", 22, MSG_CONFIRM);
        } else{
          send(sock, "250 Command completed\r\n", 23, MSG_CONFIRM);
        }
        return cd(pathname);         
    case STRU:
      n = changeStruc(sesh, tokens);
      if(n == -1){
        send(sock, "504 Parameter list error\r\n", 26,MSG_CONFIRM);
        return -1;// hmmm
      }
      else if (n == 1){
        send(sock, "504 Filetype not implemented\r\n", 30,MSG_CONFIRM);
        return -1;
      }
      return 0;
    case RETR: //Connect data stream to client, xfer from server to client using current parameters
      datafd = connect_to_client(sesh);
      // transferFile(Session, char * filename, int sDataSocket, int control);
      if(transferFile(sesh, tokens[1], datafd, sock) < 0) {
        close(datafd);
      }
      return 0;
    case STOR://connect data strem to client, xfer from client ot server using current parameters
      
      datafd = connect_to_client(sesh);
      // for(unsigned int v = 0; v < numtokens; v++)
      //   fprintf(stderr, "%d: %s\n", v, tokens[v]);
      // transferFile(Session, char * filename, int sDataSocket, int control);
      if(storeFile(sesh, tokens[1], &datafd, sock) <= 0) {
        close(datafd);
        return 0;
      }
      return -1;

    case NOOP:
      n = send(sock,"220 Service ready\r\n",19, MSG_CONFIRM);
      if (n < 0){
        error("send()");
      }   

      return 0;           
    default:
      #ifdef DEBUG
      fprintf(stderr,"Default Command case reached. Check\n");
      fflush(stderr);
      #endif
      return -1;
  }
}

int freeTokens(Tokens in, size_t numtokens){
  for (size_t i = 0; i < numtokens; i++) {
            free(in[i]);
  }
  if (in != NULL)
      free(in);
    return 1;
}

// Tokens strsplit(const char * str, const char * delim, size_t * numtokens){
//   char * s = strdup(str);
//   size_t tokens_alloc = 1;
//   size_t tokens_used = 0;
//   char ** tokens = calloc(tokens_alloc, sizeof(char*));
//   char * token, *rest = s;
//   while((token = strsep(&rest, delim)) != NULL)
//   {
//     if(tokens_used == tokens_alloc)
//     {
//       tokens_alloc *= 2;
//       tokens = realloc(tokens, tokens_alloc * sizeof(char *));
//     }
//     tokens[tokens_used++] = strdup(token);
//   }
//   if (tokens_used == 0){
//     free(tokens);
//     tokens = NULL;
//   } else {
//     tokens = realloc(tokens, tokens_used * sizeof(char*));
//   }
//   *numtokens = tokens_used;
//   free(s);
//   return tokens;
// }

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
    char **tokens = calloc(tokens_alloc, sizeof(char*));
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