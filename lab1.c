#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAX_WORDS 16
#define MAX_LINE 80

int main()
{
 int inword = 0, i = 0, j = 0, ch;
 char buffer[MAX_WORDS][MAX_LINE];
 char* argv[MAX_WORDS];

 printf("$");
 
 while ((ch = getchar ()) != EOF) {
  if (ch == ' ' || ch == '\n')  {
     if (inword == 1) {
        inword = 0; 
        buffer[i][j] = '\0'; 
        argv[i] = buffer[i]; 
        ++i; 
        j = 0;
     }
  }
     else {
       buffer[i][j] = ch; 
       ++j; 
       inword = 1;
     }
     if (ch == '\n')  {
        argv[i] = NULL;
        pid_t pid = fork();
        if (!pid) { 
           // child branch
           int rv = execvp(argv[0], argv);
           if (rv == -1) {
             perror("execvp");
             return EXIT_FAILURE;
           }
         }
         // parent branch
         pid = wait(NULL);
         if (pid == -1) {
           perror("wait");
           return EXIT_FAILURE;
         }      
         inword = 0; 
         i = 0; 
         j = 0;
         printf("$");   
  }
 
 }
 printf("\n");
return EXIT_SUCCESS; 
}
