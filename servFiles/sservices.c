#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <dirent.h>
#include <arpa/inet.h>
#include "sservices.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int cd(char * c){
  return chdir(c);
}

void handler(int n){
  
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

int asciinpRead(char * buffer, const int BUFFER_SIZE, FILE * stream){
  int c;
  int i = 0;
  for(i = 0; i < BUFFER_SIZE -1; i++){
    c = fgetc(stream);
    if(c == '\n'){
      buffer[i++] = '\r';
      buffer[i] = '\n';
      //break;
    }else if (feof(stream)){
      fprintf(stderr, "EOF\n");
      //i--;
      break;
    } else if (c == '\0'){
        perror("getc()");
        return -1;
    } else{
      buffer[i] = c;
    }
  }
  return i;
}

//Only call AFTER connection is established
int transferFile(Session * sesh, char * filename, int sDataSocket, int control)
{
    int  maxfd;
    size_t n, rt, retval;  //TODO: Simplify
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
    if(!fp) {
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
      n = asciinpRead(buffer, BUFFER_SIZE, fp);
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
      if(retval == -1){fclose(fp); perror("select()");}
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
    printf("Connected to %d on port %d\n", ntohs(client_addr->sin_port), _PORT_LD);
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

char *  list_dir(int argc, char * path)
{
  DIR *dp;
  struct dirent *ep, *result;  
  int rt; 
  char * dirpath;
  if (argc > 0)
    dirpath = strdup(path);
  else
    dirpath = "./"; 

  dp = opendir (dirpath);
  if(!dp){
    free(dirpath);
    perror("Path not valid.");
  }
  int name_max = pathconf(dirpath, _PC_NAME_MAX);
   if (name_max == -1)         /* Limit not defined, or error */
       name_max = 255;         /* Take a guess */
  int len = offsetof(struct dirent, d_name) + name_max + 1;
  ep = malloc(len);
  if (dp != NULL)
  {
    int i = 0;
    while (1){
      rt = readdir_r (dp, ep, &result);
      if(rt != 0){perror("Error reading file");}
      else if (result == NULL) break;
      else printf("%*.*s\t",-15, 15, ep->d_name/*, ep->d_type*/);
      i = (i+1) % 4;
      if(i == 0) printf("\n");
    }
    printf("\n");
    (void) closedir (dp);
  }
  else
    perror ("Couldn't open the directory");

  return 0;
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
    sprintf(notImplemented, "502 %s  not implemented\r\n", "CWD");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
  }else if(strncmp(buffer[0], "LIST", 4) == 0){
    sprintf(notImplemented, "502 %s not implemented\r\n", "LIST");
    n = send(sock,notImplemented,sizeof(notImplemented), MSG_CONFIRM);
    if(n < 0) perror("Send err failed");
    return INVALID;
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