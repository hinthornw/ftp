What I need for client:

  5.1.  MINIMUM IMPLEMENTATION

      In order to make FTP workable without needless error messages, the
      following minimum implementation is required for all servers:

         TYPE - ASCII Non-print
         MODE - Stream
         STRUCTURE - File, Record
         COMMANDS - USER, QUIT, PORT,
                    TYPE, MODE, STRU,
                      for the default values
                    RETR, STOR,
                    NOOP.

      The default values for transfer parameters are:

         TYPE - ASCII Non-print
         MODE - Stream
         STRU - File

      All hosts must accept the above as the standard defaul

  Flags: 

USER <SP> <username> <CRLF>
            PASS <SP> <password> <CRLF>
            ACCT <SP> <account-information> <CRLF>
            CWD  <SP> <pathname> <CRLF>
            CDUP <CRLF>
            SMNT <SP> <pathname> <CRLF>
            QUIT <CRLF>
            REIN <CRLF>
            PORT <SP> <host-port> <CRLF>
            PASV <CRLF>
            TYPE <SP> <type-code> <CRLF>
            STRU <SP> <structure-code> <CRLF>
            MODE <SP> <mode-code> <CRLF>
            RETR <SP> <pathname> <CRLF>
            STOR <SP> <pathname> <CRLF>
            STOU <CRLF>
            APPE <SP> <pathname> <CRLF>
            ALLO <SP> <decimal-integer>
                [<SP> R <SP> <decimal-integer>] <CRLF>
            REST <SP> <marker> <CRLF>
            RNFR <SP> <pathname> <CRLF>
            RNTO <SP> <pathname> <CRLF>
            ABOR <CRLF>
            DELE <SP> <pathname> <CRLF>
            RMD  <SP> <pathname> <CRLF>
            MKD  <SP> <pathname> <CRLF>
            PWD  <CRLF>
            LIST [<SP> <pathname>] <CRLF>
            NLST [<SP> <pathname>] <CRLF>
            SITE <SP> <string> <CRLF>
            SYST <CRLF>
            STAT [<SP> <pathname>] <CRLF>
            HELP [<SP> <string>] <CRLF>
            NOOP <CRLF>


Client: First

1) Command line call =>./program host 
-> if receive appropriate call -> "connected to %s" Val
-> server listed on port 21, then established a NEW socket on defined port
2) Username: (input)
-> Send USER blabla
3) Password: () // TODO hide text
-> send PASS blabla
4) What file would you like to access? 



ftp -h, --help
	"Usage ftp (host)\ne.g. ftp localhost 21"


filezilla -s, --site-manager
filezilla -c, --site=<string>
filezilla -l, --logontype=<string>
filezilla -a, --local=<string>
filezilla --close

filezilla --verbose
filezilla -v, --version 

Not helpful..

wftp
    -> Goes into the "shell" version -> has a number of options to choose from

wftp -help
    -> print out the usage

wftp abc.dfas.edu
    -> attempt to connect (over telnet);

wftp abc.dfas.edu 099392

general flow when connected:

1) wait for stdin input
2) check if legal command ()


TODO: 
1) Isolate data connection function
2) Implement download and upload functions
3) allow for transferral of names
    -> Reread the ftp protocol for this
    i.e. RETR "filename"
          Send the request
          receive a 150 File status OK
          Open the file for writing
              -Meanwhile the server opens for reading
          transfer

      This way the name is already on both sides