#include <stdio.h>
#include <string.h>

extern void textrun(char *,unsigned int);

int main(int argc, char **argv)
{
char t[255];
const char lcd_home[3]={27,'h',0};
const char lcd_clear[3]={27,'c',0};
char text[255];
char print[21];
int t1,t2,t3;
int rt1,rt2,rt3;

  if (argv[1]!=NULL && strlen(argv[1])!=0) {
    strcpy(text,argv[1]);
    strcat(text,"   ");
    strcpy(print,argv[1]);
  } else
    strcpy(text,".Playing unknown MP3..");
  printf("%s",lcd_clear);
  while (!feof(stdin)) {
    scanf("%s",t);
/*    printf("%s\n",t);*/
    if (sscanf(t,"%d:%d.%d",&t1,&t2,&t3)==3) {
      printf("%sTime: %02d:%02d.%02d\n",lcd_home,t1,t2,t3);
      fflush(stdout);
    } else
    if (sscanf(t,"[%d:%d.%d]",&rt1,&rt2,&rt3)==3) {
      printf("Rest: %02d:%02d.%02d\n",rt1,rt2,rt3);
      if (strlen(text)>14) {
        textrun(text,1);
        print[0]='\0';
        strncat(print,text,14);
      }
      printf("Title %s\r",print);
      fflush(stdout);
    }
  }

return 0;
}
