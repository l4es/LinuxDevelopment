#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
 
#include "lcd.h"

int main (int argc, char* argv[])
{
	int lcd;
	int ret;

	lcd = open("/dev/lcd",O_RDWR);
	ret = ioctl(lcd,IOC_CMD,'b');
	sleep(1);
	ret = ioctl(lcd,IOC_CMD,'c');
	sleep(1);
	ret = ioctl(lcd,IOC_CMD,'R');
	sleep(1);
	ret = ioctl(lcd,IOC_CMD,'B');
	sleep(1);
	ret = ioctl(lcd,IOC_CMD,'k');
	sleep(1);
	ret = ioctl(lcd,IOC_CMD,'K');
	sleep(1);

	
	
	/* infinite loop (use ^C to abort) */
	while ( 1 )
	{
			unsigned char event;

			printf("waiting for key event\n");
			ret = ioctl(lcd,IOC_WAIT_KEY,&event);
			if ( event & 0x10 )
			{
					printf("pressed %c\n", 'A' + (event & 0x0F));
			}else{
					printf("released %c\n", 'A' + (event & 0x0F));
			}
	} 
	return 0;
}
