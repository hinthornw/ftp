#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <sys/types.h>
#include <dirent.h>
//DIR * opendir(const char * name);
//struct dirent * readdir(DIR * dirp);
//
int main (int argc, char *argv[])
{
  DIR *dp;
  struct dirent *ep, *result;  
  int rt; 
  char * dirpath;
  if (argc > 1)
    dirpath = strdup(argv[1]);
  else
    dirpath = "./"; 

  dp = opendir (dirpath);
  if(!dp){
    free(dirpath);
    perror("Path not valid.");
  }
  int name_max = pathconf(dirpath, _PC_NAME_MAX);
   if (name_max == -1)         /* Limit not defined, or error */
       name_max = 255;         /* Take a guess */
  int len = offsetof(struct dirent, d_name) + name_max + 1;
  ep = malloc(len);
  if (dp != NULL)
  {
    int i = 0;
    while (1){
      rt = readdir_r (dp, ep, &result);
      if(rt != 0){perror("Error reading file");}
      else if (result == NULL) break;
      else printf("%*.*s\t",-15, 15, ep->d_name/*, ep->d_type*/);
      i = (i+1) % 4;
      if(i == 0) printf("\n");
    }
    printf("\n");
    (void) closedir (dp);
  }
  else
    perror ("Couldn't open the directory");

  return 0;
}

