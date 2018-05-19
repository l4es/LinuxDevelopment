#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/isdn.h>
#include <linux/errno.h>
#include <sys/stat.h>
#include <string.h>

/*
Credits

Die Berechnung des ISDN-Durchsatzes wurd von kisdn (PPPStatus class) kopiert.
kisdn wurde geschrieben von Thorsten Westheider and Carsten Pfeiffer.
Updates zu kisdn gibt es unter http://www.kde.org

Der Rest des Programms ist eine anpassung von proclcd von Nils Faerber.
http://www.home.unix-ag.org/nils/lcd.html

Ich (Juergen Bauer) bin per email erreichbar unter jb@idowa.de
 5. August 2000
*/

const char bar0[]={27,0,16,16,16,16,16,16,16,16};
const char bar1[]={27,1,24,24,24,24,24,24,24,24};
const char bar2[]={27,2,28,28,28,28,28,28,28,28};
const char bar3[]={27,3,30,30,30,30,30,30,30,30};
const char bar4[]={27,4,31,31,31,31,31,31,31,31};
const int NO_CHANNELS = 2;
const int DELAY_SECONDS = 1;

extern int errno;
char *strerror(int errnum);

void loadbar(char *t,float bar)
{
static char bbuf[22];
float fload=0;
float res=0;
int i;
  fload = bar;
  bbuf[14]='\0';
 
  if (fload >= 14) fload=14;
  for (i=0;i<14;i++) bbuf[1]=32;
  for (i=0;i<(fload-1);i++) bbuf[i]=255;
  res=fload-i;
  res*=4;
  bbuf[i]=(unsigned char)res;
  for (i=(fload+1);i<14;i++) bbuf[i]=' ';
  if (fload==14) bbuf[13]='+';
  printf("%1s%5.2f%s",t,bar,bbuf);
 
}


void sonst(float i_rate ,float o_rate)
{
char buf[30],buf2[30],buf3[30],tbuf[30];
char *bp;
int fd,bla;
time_t mtime;
struct tm *mmtime;

  fd=open("/proc/loadavg",O_RDONLY);
  bla=read(fd,buf,255);
  close(fd);
  buf[bla-1]='\0';
  if ((bp=strtok(buf," "))!=NULL) strcpy(buf2,bp);
  if ((bp=strtok(NULL," "))!=NULL) strcpy(buf3,bp);
  mtime=time(NULL);
  mmtime=localtime(&mtime);
  strftime(tbuf,255,"%d.%m.%y %H:%M.%S",mmtime);
  
    
  /*Zeile 1*/
  printf("%ch%cc%s",27,27,tbuf);
  /*Zeile 2*/
  printf("%ch\nLoad: %s %s\n",27,buf2,buf3);
  fflush(stdout);
  /*Zeile 3*/
  printf("%ch\n\n",27);
  loadbar("I",i_rate);
  fflush(stdout);
  /*Zeile 4*/
  printf("%ch\n\n\n",27);
  loadbar("O",o_rate);
  fflush(stdout);

}


int main(void)
{

typedef struct 
{
  ulong ibytes;
  ulong obytes;
} Siobytes;


static Siobytes ioBytes[ISDN_MAX_CHANNELS], tBytes[16];

float Rate, iRate[2], oRate[2] ,deltaKBytes, secondsPassed, giRate, goRate;

struct timeval TimeNow, TimeLast;
int f_isdn;  
ushort i;

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
     

/* init */
  f_isdn = open("/dev/isdninfo", O_RDONLY | O_NDELAY );
  if (f_isdn < 0) 
  { 
    perror("Can't open /dev/isdninfo"); 
  }

  for (i = 0; i < NO_CHANNELS; i++)
  {
    ioBytes[i].ibytes = 0;
    ioBytes[i].obytes = 0;
    tBytes[i].ibytes = 0;
    tBytes[i].obytes = 0;   
  }

  gettimeofday(&TimeLast, NULL); 
  TimeNow = TimeLast;
  close(f_isdn);

  do 
  {
  /* Berechnung */  
      gettimeofday(&TimeNow, NULL);

      secondsPassed   = (float)
       ((TimeNow.tv_sec  )+(TimeNow.tv_usec  / 1000000))
      -((TimeLast.tv_sec )+(TimeLast.tv_usec / 1000000));
 
      TimeLast = TimeNow; 
      f_isdn = open("/dev/isdninfo", O_RDONLY | O_NDELAY );
  
      if (f_isdn < 0) 
      { 
	fprintf(stderr, "Can't open /dev/isdninfo"); 
      }
  
      if ( ioctl(f_isdn, IIOCGETCPS, ioBytes) == 0 ) 
      {
	for ( i = 0; i < NO_CHANNELS; i++)
	{
    	    deltaKBytes = (float) (ioBytes[i].ibytes - tBytes[i].ibytes)/1024;
	    Rate = (float) deltaKBytes / secondsPassed;
	    
	    if (secondsPassed && (Rate < 30))
		     {	iRate[i] = Rate;}
	    else     {	iRate[i] = 0;}  
	    tBytes[i].ibytes = ioBytes[i].ibytes;

	    deltaKBytes = (float) (ioBytes[i].obytes - tBytes[i].obytes)/1024;
            Rate = (float) deltaKBytes / secondsPassed;
	    if (secondsPassed && (Rate < 30))
		     {	oRate[i] = Rate;}
	    else     {	oRate[i] = 0;}  
	    tBytes[i].obytes = ioBytes[i].obytes;
	}
      }
      else   
      {        
        perror ("IOCTL");
      	fprintf(stderr, "IOCTL error (IIOCGETCPS)\n");        
      }
     close (f_isdn);
     giRate = 0;
     goRate = 0;
     
     /*Raten aller Kanaele addieren*/
     for ( i = 0; i < NO_CHANNELS; i++)
     {
      giRate = giRate + iRate[i];
      goRate = goRate + oRate[i];
      
     }
     sonst(giRate,goRate); 
  
     sleep(DELAY_SECONDS);

 } while ( 1 );

}
