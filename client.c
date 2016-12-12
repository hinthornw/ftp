#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
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


enum State{NOTREADY, NEEDUSER, NEEDPASS, LOGGEDIN, CLOSED, WAIT};
void error(const char *msg)
{
    perror(msg);
    exit(0);
}
int login(int fd, /*char * buffer,*/ char * connection, fd_set * rfds, enum State);
int main(int argc, char *argv[])
{
    int sockfd, portno, n, i;
    const int TELNET = 21;
    enum State state = NOTREADY;
    fd_set rfds; // File descriptors to be read
    struct timeval tv; // Timer for read value
    int retval; // Return value for the timeval
    int ret; // return value for login
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int maxfd;

    char buffer[256];
    char formatted[300];
    if (argc != 2) {
       fprintf(stderr,"usage %s hostname\n", argv[0]);
       exit(0);
    }
    portno = TELNET;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    maxfd = sockfd+1;
    //Initialize the to read FD.
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);

    //Get the IP address
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    //TODO: if client wants to specify port number, need way to BIND...
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

   
   

    i = 0;
    while(i < 20){
        if(state == NEEDUSER){
            ret =  login(sockfd, /*buffer,*/ argv[1], &rfds, state);
            if(ret){error("LOGIN FAILED");}
        }
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        bzero(buffer, sizeof(buffer));
        retval = select(maxfd, &rfds, NULL, NULL, &tv);
        if(retval == -1)error("select()");
        else if (retval)
        {
            n = recv(sockfd,buffer,255, 0);
            if (n < 0) 
                 error("ERROR reading from socket");
            if(n != 0){
                printf("%d: %s\n",n, buffer);
                if(strncmp(buffer, "QUIT", 4) == 0){
                    printf("QUIT received\n");
                    break;
                }
                i++;
            } 
        }
        else
        {
            printf("Timeout\n");
            break;
        }

        if(state == NOTREADY) // Awaiting 220 Service Ready from server
        {
            if(strncmp(buffer, "220", 3) == 0){
                state = NEEDUSER;
            }
            else{
                error("System not ready, blocking.\n");
            }
        }
        printf("ftp$ ");
    }
    close(sockfd);
    return 0;
}



int login(int fd, /*char * buffer, */char * connection, fd_set * rfds, enum State state){
    char buffer[256];
    char formatted[300];
    enum State oldState = NEEDUSER;
    int n, retval;
    int maxfd = fd + 1;
    struct timeval tv;
    //Input User
    printf("Connected to %s\n", connection);
    while(1){
        if (state == NEEDUSER)
        {
            printf("Username: ");
            bzero(buffer, sizeof(buffer));
            bzero(formatted, sizeof(formatted));
            fgets(buffer, sizeof(buffer) - 1,stdin);
            sprintf(formatted, "USER %s\r\n", buffer);
            n = send(fd,formatted,strlen(formatted), MSG_CONFIRM);
            if (n < 0) 
                 error("ERROR writing to socket");
            bzero(buffer,256);
            oldState = state;
            state = WAIT;
        }
        else if (state == NEEDPASS)
        {
            bzero(buffer, 256);
            printf("Password: ");
            fgets(buffer,255,stdin);
            bzero(formatted, 300);
            sprintf(formatted, "PASS %s\r\n", buffer);
            n = send(fd, formatted, strlen(formatted), MSG_CONFIRM);
            oldState = state;
            state = WAIT;
        }
        else if(state == WAIT)
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
                    printf("%d: (%s)\n",n, buffer);
                    // printf("%d\tTier1:%d\tTier2:%d \n", oldState, strncmp(buffer, "331", 3), strncmp(buffer, "230", 3));
                    if(strncmp(buffer, "QUIT", 4) == 0){
                        printf("QUIT received\n");
                        close(fd);
                        return 1;
                    } else if (oldState == NEEDUSER && strncmp(buffer, "331", 3) == 0){
                       printf("User OK, Input Password.\n");
                       state = NEEDPASS;
                    } else if (oldState == NEEDPASS && strncmp(buffer, "230", 3) == 0){
                        printf("Login success. returning.\n");
                        return 0;
                    }
                    else{
                        printf("unaccuonted for. Exiting\n");
                        close(fd);
                        return 1;
                    }

                } 
            }
            else
            {
                printf("Timeout received. Exiting.\n");
                close(fd);
                return 1;
            }

        }
        else
        {
            printf("This makes 0 sense.\n");
        }
        
            
    }

}