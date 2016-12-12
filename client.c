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
    



void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n, i;
    const int TELNET = 21;
    fd_set rfds; // File descriptors to be read
    struct timeval tv; // Timer for read value
    int retval; // Return value for the timeval
    struct sockaddr_in serv_addr;
    struct hostent *server;


    char buffer[256];
    char formatted[300];
    if (argc < 2) {
       fprintf(stderr,"usage %s hostname\n", argv[0]);
       exit(0);
    }
    portno = TELNET;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
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
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    printf("Connected to %s\n", argv[1]);
    printf("Username: ");
    bzero(buffer,256);
    bzero(formatted, 300);
    fgets(buffer,255,stdin);
    sprintf(formatted, "USER %s\r\n", buffer);
    n = send(sockfd,formatted,strlen(formatted), MSG_CONFIRM);
    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,256);
    //Wait for response;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    retval = select(1, &rfds, NULL, NULL, &tv);
    if(retval == -1)error("select()");
    else if (retval)
    {
        n = recv(sockfd,buffer,255, MSG_PEEK);

        // if (n < 0) 
        //      error("ERROR reading from socket");
        if(n != 0){
            printf("%d: %s\n",n, buffer);
            if(strncmp(buffer, "QUIT", 4) == 0){
                printf("QUIT received\n");
                close(sockfd);
                return 1;
            } else{
                bzero(buffer, 256);
                printf("Password: ");
                fgets(buffer,255,stdin);
                bzero(formatted, 300);
                sprintf(formatted, "PASS %s\r\n", buffer);
                n = send(sockfd, formatted, strlen(formatted), MSG_CONFIRM);

            }
        } 
    }
    else
    {
        printf("Timeout received. Exiting.\n");
        close(sockfd);
        return 1;
    }


    i = 0;
    while(i < 5){
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        retval = select(1, &rfds, NULL, NULL, &tv);
        if(retval == -1)error("select()");
        else if (retval)
        {
            n = recv(sockfd,buffer,255, MSG_PEEK);

            // if (n < 0) 
            //      error("ERROR reading from socket");
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
    }
    close(sockfd);
    return 0;
}