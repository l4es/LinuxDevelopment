#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../driver/hardware.h"

int main(int argc, char **argv)
{
int dev,i,b;
char buf[LCD_COLS*LCD_LINES];

  dev=open("/dev/lcd",O_WRONLY|O_NDELAY);
  if (dev==-1) {
    fprintf(stderr,"open() failed\n");
    exit(-1);
  }
  buf[0]=27;
  buf[1]='c';
  write(dev,buf,2);
  for (i=1;i<=(LCD_COLS*LCD_LINES);i++) buf[i-1]=(i%10)+48;
  write(dev,buf,LCD_COLS*LCD_LINES);
  printf("You should see numbers in all rows now. (Press any key when ready)\n");
  getchar();
  printf("And now some screens with the whole font ;) Interrupt to end...\n");
  while(1) {
    for (i=0; i<224; i++) {
      for (b=0;b<(LCD_COLS*LCD_LINES);b++) buf[b]=i+32;
/*      sprintf(buf,"%d",i);*/
      write(dev,buf,(LCD_COLS*LCD_LINES));
      usleep(500000);
    }
  }
  close(dev);
 
return(0);
}