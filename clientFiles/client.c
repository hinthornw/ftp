#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "cservices.h"

    // struct timeval {
    //     long tv_sec;  seconds 
    //     long tv_usec; /* microseconds*/
    // }
//     struct  hostent{
//   char    *h_name;        /* official name of host */
//   char    **h_aliases;    /* alias list */
//   int     h_addrtype;     /* host address type */
//   int     h_length;       /* length of address */
//   char    **h_addr_list;  /* list of addresses from name server */
//   #define h_addr  h_addr_list[0]  /* address, for backward compatiblity */
// };

typedef enum {mSTREAM} mode;
typedef enum {sFILE, sRECORD} structure;
typedef enum {cASCII} codeType;
typedef struct ftpState{
    char * user;
    char * pw;
    mode m;
    structure s;
    codeType cT;
} ftpState;


int main(int argc, char *argv[])
{
    int sockfd, /*portno,*/ i;
    enum State state = NOTARGET;
    fd_set rfds; // File descriptors to be read
    int ret; // return value for login
    const int BUFFER_SIZE = 256;
    struct sockaddr_in serv_addr;
    struct hostent server;
    char buffer[BUFFER_SIZE];
    bzero(buffer, sizeof(buffer));

    //Check type of command: could be:
        //a) Open wiftp - (interact from there)
        //b) Connect to [HOST]
        //c) Connect to [HOST], data on port [PORT]
        //d) Error
    if (argc > 3) { // ERROR
       fprintf(stderr,"usage: %s [host] [PORT]\n", argv[0]);
       exit(0);
    } else if(argc == 3){ // C
        printf("Connecting to %s. Request data connect on port %d\n", argv[1], atoi(argv[2]));
        //TODO create a connect_to_host including a port request (i.e. bind instead of connect straight)
    }
    else if (argc == 2){     // B
        printf("Connecting to %s\n", argv[1]);
        sockfd = connect_to_host(argv[1], &serv_addr, &server, &rfds);
        if(sockfd < 0) error("Exiting.");
        else
            state = NOTREADY;
    } else{ // A TODO
        printf("%d args\n", argc);
    }
   
   

    i = 0;
    while(i < 20){
        if(state == NEEDUSER){
            ret =  login(sockfd, /*buffer,*/ &rfds, &state);
            if(ret){error("LOGIN FAILED");}
        } else if(state == NOTREADY) // Awaiting 220 Service Ready from server
        {
            ret = waitForMessage(sockfd, buffer, BUFFER_SIZE, &rfds);
            if(strncmp(buffer, "220", 3) == 0){
                state = NEEDUSER;
            }
            else{
                error("System not ready, blocking.\n");
            }
        } else if (state == NOTARGET){
            printf("ftp> TODO: STUFF - State NOTARGET not finished");
            //getline();
        }
        // printf("ftp$ ");

        //ret = waitForMessage(sockfd, buffer, &rfds);

       
    }
    close(sockfd);
    return 0;
}



// FUnctions I need:

// 1) login
// 2) cd
// 3) Parameters
//     - Port, mode, structure

// 4) Send File
// 5) ls 
// 6) Download from server 
// 7) NOOP 
// 8) Print help
// 9) Parse stdin input
// 10) parse server input (multiple lines)

// USER, QUIT, PORT,
//                     TYPE, MODE, STRU,
//                       for the default values
//                     RETR, STOR,
//                     NOOP.