#ifndef _sservicesH
#define _sservicesH

#define DEBUG

enum servState{NEEDUSER, NEEDPASS, LOGGEDIN, CLOSED};
//State - Closed, Listening, 
enum Type{ASCIINP};
enum Mode{STREAM};
enum Structure{sFILE, sREC};
typedef char ** Tokens;
typedef struct session{
  char usr[256];
  char psw[256];
  int serv_data_portno;
  struct sockaddr_in  c_control;
  struct sockaddr_in  c_data;
  enum Mode mode;
  enum Type type;
  enum Structure structure;
} Session;

enum ftpCommands{INVALID, RETR, STOR, STOU, APPE, ALLO, REST, RNFR, RNTO, ABOR, DELE, 
					RMD, MKD, PWD, LIST, NLST, SITE, SYST, HELP, NOOP, USER, 
					PASS, ACCT, CWD, CDUP, SMNT, REIN, QUIT, PORT, PASV, TYPE, MODE, STRU};

extern const int _PORT_L;

int transferFile(Session * sesh, char * filename, int sDataSocket, int control);
int connect_to_client(Session * sesh);
void handler(int n);
int login(Session * , int, char *);
int cd(char *);
void error(const char *msg);
int changePort(Session * sesh, Tokens tok);
enum ftpCommands checkMessage(int sock, Tokens buffer);
Tokens strsplit(const char * str, const char * delim, size_t * numtokens);
int freeTokens(Tokens in, size_t numtokens);
#endif // _sservicesH
