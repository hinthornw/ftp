#ifndef _cservicesH
#define _cservicesH

enum State{NOTARGET, NOTREADY, NEEDUSER, NEEDPASS, LOGGEDIN, CLOSED, WAIT};

int waitForMessage(int sockfd, char * buffer, const int bufferSize, fd_set * rfds);
int login(int fd, /*char * buffer,*/ fd_set * rfds, enum State * state);
int connect_to_host(char * host, struct sockaddr_in * serv_addr, struct hostent * server, fd_set * rfds);
void error(const char *msg);
#endif // _cservicesH