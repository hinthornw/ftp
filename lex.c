#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <termios.h>

// typedef struct token{
// 	char * text;
// 	struct token * next;
// } token;

// typedef struct tokenList{
// 	int length;
// 	struct token * head;
// } tokenList;

// tokenList * lex(char * input){
// 	tokenList * ls = (tokenList *) calloc(1, sizeof(tokenList));
// 	token * temp1, * temp2;
// 	char buffer[256];

// 	while(1){

// 	}


// }

//Adapted from https://www.quora.com/How-do-you-write-a-C-program-to-split-a-string-by-a-delimiter


// int getcha(void)
// {
// 	struct termios oldattr, newattr;
// 	int ch;
// 	tcgetattr(STDIN_FILENO, &oldattr);
// 	newattr = oldattr;
// 	newattr.c_lflag &= ~( ICANON | ECHO); //
// 	tcsetattr( STDIN_FILENO, TCSANOW, &newattr);
// 	system("stty -echo");  // kill the echo with a shell out?
// 	ch = getchar();
// 	system("stty echo");
// 	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
// 	return ch;

// }

int main(int argc, char **argv){
    int i=0;       
    char d,c='h',s[6];
    printf("Enter five secret letters\n");
    s[5]='\0'; //Set string terminator for string five letters long
    do{
      d=getcha();//Fake getcha command silently digests each character
    printf("*");
      s[i++]=d;} while (i<5);
    printf("\nThank you!!\n Now enter one more letter.\n");
    c=getchar();
    printf("You just typed %c,\nbut the last letter of your secret string was %c\n",c,d);   
    printf("Your secret string was: %s\n",s);   
        return 0;
    }








// 	return 0;
// }