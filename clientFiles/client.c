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



int main(int argc, char *argv[])
{
    int sockfd, /*portno,*/ i;
    enum cmdType cmd = INVALID;
    fd_set rfds; // File descriptors to be read
    int ret, datafd = -1; // return value for login
    size_t BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE];
    Tokens tokens;
    size_t numTokens;
    bzero(buffer, sizeof(buffer));
    Session sesh;
    int _PORT_L = 21;
 
    /** partially initialize **/
    bzero((char *) &sesh, sizeof(sesh));
    sesh.serv_data_portno = _PORT_L - 1;
    sesh.mode = STREAM;
    sesh.type = ASCIINP;
    sesh.structure = sFILE;
    sesh.childpid = -1;
    sesh.controlfd = -1;
    sesh.datafd = -1;
    sesh.state = NOTARGET;

    
    if (argc >= 3) { // ERROR
       fprintf(stderr,"usage: %s [host]\n", argv[0]);
   } else if (argc == 2){     // B
        printf("Connecting to %s\n", argv[1]);
        sockfd = connect_to_host(argv[1], &sesh, &rfds);
        if(sockfd == -2) error("Exiting.");
        else if (sockfd == -1){
            sockfd = connect_to_host(argv[1], &sesh, &rfds); // Try twice
            if (sockfd < 0) error("connect()");
        }
        else
            sesh.state = NEEDUSER;
    } 
   
   

    i = 0;
    //In general, handlers will reply -1 for temp failure, -2 for closing..
    while(i++ < 30){
        bzero(buffer, BUFFER_SIZE);
        switch(sesh.state){
            case NEEDUSER:
                ret =  login(sockfd, &sesh, &rfds);
                if(ret){
                    fprintf(stderr, "LOGIN FAILED. Disconnecting\n");
                    sesh.state = NOTARGET;
                    sesh.controlfd = -1;
                    close(sockfd);
                }
                break;
            case LOGGEDIN:
                printf("ftp$ ");
                fflush(stdout);
                cmd = getCommand(buffer, BUFFER_SIZE, &tokens, &numTokens);
                if(cmd == INVALID){ // Wait for command from stdin, parse into tokens, return 
                    fprintf(stderr, "Invalid command\n");
                    //Help() usage stuff
                    freeTokens(tokens, numTokens);
                    break;
                }

                datafd = handleCommand(cmd, &sesh, tokens, &numTokens) == -1;
                if(datafd == -1){
                    fprintf(stderr, "Error");
                }
                freeTokens(tokens, numTokens);
                break;
            case NOTARGET: // ftp with no connection i.e. DISCONNECTED
                printf("NTftp$ ");
                fflush(stdout);
                if((cmd = getCommand(buffer, BUFFER_SIZE, &tokens, &numTokens)) == INVALID){ // Wait for command from stdin, parse into tokens, return 
                    fprintf(stderr, "Invalid command\n");
                    //Help() usage stuff
                    freeTokens(tokens, numTokens);
                    break;
                }
                else if (cmd != HELP){
                    freeTokens(tokens, numTokens);
                    printf("No connection. To connect, type open [host]\n");
                }else{
                    freeTokens(tokens, numTokens);
                    printf("HELP\n");
                }
                break;
            case WAIT: //could be transmitting?
                printf("Action in progress.\n");
                // waitforinput()
                break;

            default:
                fprintf(stderr, "Default case reached..\n");
                printf("ftp$");
                fflush(stdout);
        }
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