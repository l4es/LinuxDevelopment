#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

/*
 * (c) 1998,1999 by Nils Faerber, GNU Public License applies
 */

const char bar0[]={27,0,16,16,16,16,16,16,16,16};
const char bar1[]={27,1,24,24,24,24,24,24,24,24};
const char bar2[]={27,2,28,28,28,28,28,28,28,28};
const char bar3[]={27,3,30,30,30,30,30,30,30,30};
const char bar4[]={27,4,31,31,31,31,31,31,31,31};

void loadbar(const char *load)
{
static char bbuf[22];
float fload=0;
float res=0;
int i;

  bbuf[21]='\0';
  sscanf(load,"%f",&fload);
  if (fload>=1) fload=1;
  fload=fload*20;
  for (i=0;i<(fload-1);i++) bbuf[i]=255;
  res=fload-i;
  res*=4;
  bbuf[i]=(unsigned char)res;
  for (i=(fload+1);i<20;i++) bbuf[i]=' ';
  if (fload==20) bbuf[19]='+';
/*  write(1,bbuf,20);
  fsync(1);*/
  printf("%s",bbuf);
}



int main(int argc, char **argv)
{
char buf[30],buf2[30],buf3[30],tbuf[30];
#ifdef ISDN
char bufi[10];
int blai;
#endif
char *bp;
int fd,bla,wtime=1;
time_t mtime;
struct tm *mmtime;

  if (argc>0 && argv[1]!=NULL) wtime=atoi(argv[1]);
  printf("%cc",27);
  fflush(stdout);
  printf("%s",bar0);
  fflush(stdout);
  printf("%s",bar1);
  fflush(stdout);
  printf("%s",bar2);
  fflush(stdout);
  printf("%s",bar3);
  fflush(stdout);
  printf("%s",bar4);
  fflush(stdout);
  while(1) {
    fd=open("/proc/loadavg",O_RDONLY);
    bla=read(fd,buf,255);
    close(fd);
#ifdef ISDN
    fd=open("/var/lock/isdnppp_users",O_RDONLY);
    if (fd!=-1) {
      blai=read(fd,bufi,255);
      bufi[blai-1]='\0';
      close(fd);
    } else {
      bufi[0]='0';
      bufi[1]='\0';
    }
#endif
    buf[bla-1]='\0';
    if ((bp=strtok(buf," "))!=NULL) strcpy(buf2,bp);
    if ((bp=strtok(NULL," "))!=NULL) strcpy(buf3,bp);
    mtime=time(NULL);
    mmtime=localtime(&mtime);
    strftime(tbuf,255,"%d.%m.%y %H:%M.%S",mmtime);
    printf("%ch%s\nLoad: %s %s\n",27,tbuf,buf2,buf3);
    loadbar(buf2);
#ifdef ISDN
    printf("ISDN:%s",bufi);
#endif
    fflush(stdout);
    sleep(wtime);
  }
}
