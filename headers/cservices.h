#ifndef _cservicesH
#define _cservicesH

#define DEBUG

enum State{NOTARGET, NEEDUSER, NEEDPASS, LOGGEDIN, CLOSED, WAIT};
enum cmdType{GET, PUT, PWD, LIST, CD, HELP, QUIT, INVALID};
enum Type{ASCIINP, BINARY};
enum Mode{STREAM};
enum Structure{sFILE, sREC};

typedef struct session{
  char usr[256];
  char psw[256];
  int serv_data_portno;
  int childpid;
  int controlfd;
  int datafd;
  enum State state;
  struct sockaddr_in  c_control;
  struct sockaddr_in  c_data;
  enum Mode mode;
  enum Type type;
  enum Structure structure;
} Session;

typedef char ** Tokens;
int waitForMessage(int sockfd, char * buffer, const int bufferSize, fd_set * rfds);
int login(int fd, Session * sesh, fd_set * rfds);
int connect_to_host(char * host, Session * sesh, fd_set * rfds);
void error(const char *msg);
void dataConnect (int sock, struct sockaddr_in * client_addr, char * filename);
int downloadFile(int control, int port, char * filename);
int port(Session * sesh, int * port, FILE * stream, int download);
int handleCommand(enum cmdType cmd, Session * sesh, Tokens toks, size_t * numtokens);
int getCommand(char * buffer, size_t size, Tokens *toks, size_t * numtokens);
enum cmdType commandType(char * in);
char * getIpAddress();
Tokens strsplit(const char* str, const char* delim, size_t* numtokens);
int asciinpRead(char * buffer, const int BUFFER_SIZE, FILE * stream); 
int freeTokens(Tokens in, size_t numtokens);
int removeCharReturns(char * b, int size, int * overlap);
#endif // _cservicesH