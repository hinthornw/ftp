#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>


int connect_to_client(char * client, struct sockaddr_in * client_addr, int port, struct hostent * server, fd_set * wfds);
int transferFile(int argc, char * client, char * filename, char * opts);


int main(int argc, char *argv[]){
	if(argc != 3){
		fprintf(stderr, "Usage: xfers client filename");
		return -1;
	}
	return transferFile(argc, argv[1], argv[2], "r");
}

int transferFile(int argc, char * client, char * filename, char * opts)
{
    int sockfd, maxfd/*, portno*/;
    size_t n, rt, retval;
    fd_set wfds; // File descriptors to be read
    FILE * fp;
    struct timeval tv; // Timer for read value
    const int BUFFER_SIZE = 500;
    struct sockaddr_in serv_addr;
    struct hostent server;
    char * local_opts;
    char buffer[BUFFER_SIZE];
    bzero(buffer, sizeof(buffer));

    if(filename == NULL){
    	fprintf(stderr, "File not found.\n");
    	return -1;
    }
    if(opts != NULL){
    	local_opts = opts;
    } else {
    	local_opts = "r";
    }

    printf("Connecting to %s\n", client);
    sockfd = connect_to_client(client, &serv_addr,0,  &server, &wfds);
    if(sockfd < 0){
     perror("Exiting.");
     close(sockfd);
     return -1;
	}

    //Begin transfer by opening file
    fp = fopen(filename, local_opts); // Make this dynamic..
  	if(!fp) {
  		perror("Error opening file");	
  		fclose(fp);
  		close(sockfd);
  		return -1;
  	}
  	maxfd = sockfd+1;
  	//Initialize the to read FD.
	FD_ZERO(&wfds);
	FD_SET(sockfd, &wfds);

	int i = 0;
  	while( i++ < 1000){
		bzero(buffer,BUFFER_SIZE);
		n  = fread(buffer, 1, BUFFER_SIZE, fp);
		if(n == 0){
			printf("EOF reached. CLosing stream.\n");
			fclose(fp);
			close(sockfd);
			return 0;
		}

		tv.tv_sec = 20;
		tv.tv_usec = 0;

		retval = select(maxfd, NULL, &wfds, NULL, &tv);
	    if(retval == -1){fclose(fp); perror("select()");}
	    else if (retval) {
	        rt = send(sockfd,buffer,n, MSG_CONFIRM);
	        //NEED TO FIND A WAY TO FIND EOF
	        if (rt < 0) perror("ERROR reading from socket");
	        
	    } else {
	      fclose(fp);
	      perror("Timeout");
	      return 1;
	    }

  	}
  	return 0;
}

int connect_to_client(char * client, struct sockaddr_in * client_addr, int port, struct hostent * server, fd_set * wfds){
    int /*maxfd,*/ sockfd;
    int portno;
    const int _PORT_U = 3059;
    const int _PORT_LD = 20;
    //int option = 1;
    struct sockaddr_in data_addr;
    // struct hostent * retServer;
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
         
    //Get the IP address
    server = gethostbyname(client);
    if (server == NULL) {
        fprintf(stderr,"DNS\n");
        exit(0);
    }

    bzero((char *) client_addr, sizeof(*client_addr));
    client_addr->sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&(client_addr->sin_addr.s_addr),
         server->h_length);
    client_addr->sin_port = htons(_PORT_U/*_PORT_U*/);

       
    if ((portno = connect(sockfd,(struct sockaddr *) client_addr,sizeof(*client_addr))) < 0){
        fprintf(stderr, "Error connecting.\n");
        return -1;
    } 
    printf("Connected to %s on port %d\n", client, _PORT_LD);
        // error("ERROR connecting");

    return sockfd;
}
