#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <dirent.h>
// #include <stddef.h>
#include "cservices.h"



void error(const char *msg)
{
    perror(msg);
    exit(0);
}


int connect_to_host(char * host, struct sockaddr_in * serv_addr, struct hostent * server, fd_set * rfds){
    int /*maxfd,*/ sockfd;
    const int _PORT_L = 21;
    //int option = 1;
    //struct sockaddr_in sa;
    struct hostent * retServer;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) 
        error("ERROR opening socket");
    //maxfd = sockfd+1;
    //Initialize the to read FD.
    FD_ZERO(rfds);
    FD_SET(sockfd, rfds);

    //Get the IP address
    //server = gethostbyname(host);
    retServer = gethostbyname(host);
    if (retServer == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    else{
        *server = *retServer;
    }

    //fprintf(stderr, "IMPORTANTER: Address: (%d.%d.%d.%d)\n", (int) (retServer->h_addr)[0], (int) (retServer->h_addr)[1], (int) (retServer->h_addr)[2], (int) (retServer->h_addr)[3]);

    bzero((char *) serv_addr, sizeof(*serv_addr));
    serv_addr->sin_family = AF_INET;

    bcopy((char *)server->h_addr, 
         (char *)&(serv_addr->sin_addr.s_addr),
         server->h_length);
    serv_addr->sin_port = htons(_PORT_L);
    //fprintf(stderr,"IMPORTANT: Address = (%X)\tPort: (%d)\n", serv_addr->sin_addr.s_addr, serv_addr->sin_port);

    //TODO: if client wants to specify port number, need way to BIND...
    if (connect(sockfd,(struct sockaddr *) serv_addr,sizeof(*serv_addr)) < 0){
        fprintf(stderr, "Error connecting.\n");
        return -1;
    } 
    printf("Connected to %s\n", host);
        // error("ERROR connecting");
    return sockfd;
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
        if (n < 0) 
             error("ERROR reading from socket");
        else if(n != 0){
            printf("%d: %s\n",n, buffer);
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


int login(int fd, /*char * buffer, */ fd_set * rfds, enum State * state){
    char buffer[256];
    char formatted[300];
    enum State oldState = NEEDUSER;
    int n, retval;
    int maxfd = fd + 1;
    struct timeval tv;
    //Input User
    while(1){
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
                    printf("%d: (%s)\n",n, buffer);
                    // printf("%d\tTier1:%d\tTier2:%d \n", oldState, strncmp(buffer, "331", 3), strncmp(buffer, "230", 3));
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

}

