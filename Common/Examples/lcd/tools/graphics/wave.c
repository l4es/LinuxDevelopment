#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <math.h>

/*
 * (c) 1999 by Nils Faerber, GNU Public License applies
 */

/*
 * Defining the vertical bargraph characters
 */
const char bar0[]={27,0,0,0,0,0,0,0,0,31};
const char bar1[]={27,1,0,0,0,0,0,0,31,31};
const char bar2[]={27,2,0,0,0,0,0,31,31,31};
const char bar3[]={27,3,0,0,0,0,31,31,31,31};
const char bar4[]={27,4,0,0,0,31,31,31,31,31};
const char bar5[]={27,5,0,0,31,31,31,31,31,31};
const char bar6[]={27,6,0,31,31,31,31,31,31,31};
const char bar7[]={27,7,31,31,31,31,31,31,31,31};



int main(int argc, char **argv)
{
char buf[20][4];
unsigned int i,a,b,offs;
const double fac=(2*M_PI)/20;
double y;
double tmp;
unsigned int wtime=60000;

  if (argc>1)
    sscanf(argv[1],"%d",&wtime);

  /* Clear the display */
  printf("%cc",27);
  /* Initialize the vertical bargraph characters (0-7) */
  write(1,bar0,10);
  write(1,bar1,10);
  write(1,bar2,10);
  write(1,bar3,10);
  write(1,bar4,10);
  write(1,bar5,10);
  write(1,bar6,10);
  write(1,bar7,10);
  /* Clear the drawing buffer */
  for (a=0; a<4; a++)
    for (b=0; b<20; b++)
      buf[b][a]=' ';
  offs=0;

  /* The endless loop */
  while(1) {
    for (i=0; i<20; i++) {
      tmp=((i+offs)*fac);
      y=(sin(tmp)+1)*1.9;
       for (a=0; a<4; a++) {
         if ((int)y>a)
           buf[i][a]=255;           /* paint full block */
         else if (y>a)
           buf[i][a]=(y-(int)y)*8;  /* paint bar-graph char */
         else
           buf[i][a]=32;            /* the rest is space */
       }
    }
    /* Write the drawing buffer to the display (stdout resp.) */
    for (a=0; a<4; a++)
      for (b=0; b<20; b++)
        printf("%c",buf[b][3-a]);
    fflush(stdout);
    usleep(wtime);
    /* Move the wave one step further and wrap around */
    offs+=1;
    if (offs>19) offs=0;
  }
}
