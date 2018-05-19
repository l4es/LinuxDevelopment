#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>

#define REFRESH 5

#define LINESKIP 16
#define LCDSIZE  32

char LCDhome[]="\033c\033h";

void LCDtime(int fd,int *counter){
 char buf[LCDSIZE+1];
 struct tm *t;
 time_t tt;

 /* clear, home */
 write(fd,LCDhome,4);

 /* get time */
 tt=time(NULL);


 /* localtime */
 t=localtime(&tt);
 strftime(buf,sizeof(buf),"%H:%M %d. %b %y",t);
 if(counter)
  if( ( (*counter)++ ) & 1 ) 
   buf[2]=' ';

 /* ``beats'' */
 t=gmtime(&tt);
 snprintf(buf+LINESKIP,sizeof(buf)-LINESKIP,
          "@%05.1f  ",(t->tm_hour*60+t->tm_min)/1.44,(unsigned int)tt);

 /* Weekday */
 t=localtime(&tt); /* this is ugly, but libc returns pointer to static :-( */
 strftime(buf+LINESKIP+7,sizeof(buf)-LINESKIP-7,
          "%A        ",t);

 /* output everything */
 write(fd,buf,LCDSIZE);
};

int main(int argc,char **argv){
 int fd;
 int i=0;

 /* open device */
 if((fd=open("/dev/lcd",O_WRONLY))<0){
  fprintf(stderr,"open() returned %d\n",fd);
  return(1);
 };

 while(1){
  LCDtime(fd,&i);
  sleep(REFRESH);
 };

 return 0;
};
