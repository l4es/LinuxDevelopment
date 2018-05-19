#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>



int main (int argc, char* argv[])
{
	int lcd;
	unsigned char cmdClear[] = {0x1B,'c',0x00};
	unsigned char cmdHome[] = {0x1B,'h',0x00};
	unsigned char msgInit[] = "Keypad test";
	char buttons;
	char preStatus;
	

	lcd = open("/dev/lcd",O_RDWR);
	write(lcd,cmdClear,strlen(cmdClear));
	write(lcd,cmdHome,strlen(cmdHome));

	write(lcd,msgInit,strlen(msgInit));
	sleep(1);

	preStatus = 0;
	
	/* infinite loop (use ^C to abort) */
	while ( 1 )
	{
		/* read the status of the buttons */
		read(lcd,&buttons,1);
		
		if ( buttons != preStatus )
		{
			/* show the changes */
			preStatus = buttons;
			write(lcd,cmdClear,strlen(cmdClear));
			write(lcd,cmdHome,strlen(cmdHome));
			if ( buttons & 1 )
			{
				write(lcd,"A ",2);
			}
			if ( buttons & 2 )
			{
				write(lcd,"B ",2);
			}
			if ( buttons & 4 )
			{
				write(lcd,"C ",2);
			}
			if ( buttons & 8 )
			{
				write(lcd,"D ",2);
			}
		}
		usleep(10000);
	}
	return 0;
}
