#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

extern void textrun(char *,unsigned int);

int main(int argc, char**argv)
{
unsigned int c,i,delay;
char *text,print[21];

  if (argv[2]!=NULL && strlen(argv[2])!=0)
    delay=atoi(argv[2]);
  else
    delay=125000;
  if (argv[1]!=NULL && strlen(argv[1])!=0) {
    text=(char *)malloc(strlen(argv[1])*sizeof(char));
    strcpy(text,argv[1]);
    for (c=1; c<10; c++) {
      for (i=0; i<strlen(text); i++) {
        textrun(text,1);
        print[0]='\0';
        strncat(print,text,19);
        printf("%s\r",print);
        fflush(stdout);
        usleep(delay);
      }
    }
  }

return 0;
}
