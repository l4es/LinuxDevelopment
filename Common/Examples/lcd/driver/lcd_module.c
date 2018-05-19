/*
 *	Alpha-numeric-LCD-module driver
 *	for LCD controllers HD44780 and HD66712
 *	connected to PC compatible parallel port (see README)
 *
 *	Copyright 1998-2001 Nils Faerber <nils@kernelconcepts.de>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Changes:
 *	1998		Initial release, Nils Faerber
 *	1999		Fixes, updates for kernel 2.1/2.2, Nils Faerber
 *	20000826	Added module parameters and defaults
 *			Juergen Bauer
 *	20001225	Updated driver to kernel 2.4, some cleanups/fixes
 *			Added proper printk KERN_* log-values
 *			Added new controller type (a_type==1)
 *			Nils Faerber <nils@kernelconcepts.de>
 *	20010122	Gotoxy from Ulf Lanz included
 *      		Mode for 24 Cols added
 *                      Juergen Bauer jb@idowa.net
 *	20010123	bugfix: new_dline, new_dcol
 *			Juergen Bauer 
 */

#include <linux/ioport.h>

#ifdef KERNEL20
#include <asm/segment.h>
#include <linux/sched.h>
#endif
#if defined KERNEL21 || defined KERNEL24
#include <asm/uaccess.h>
#endif

#include <asm/io.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/delay.h>
/*#include <string.h>*/
#include "version.h"
#include "c_table.h"


#include "lcd.h"

#define MSECS_TO_JIFFIES(ms) ((ms*HZ + 1000 -HZ)/1000)

/*
 * Some globals
 */
static int lcd_usage=0;
static char dline=0,dcol=0,new_dline=0,new_dcol=0;
/* By default no keypad */
static int keys = 0;
static unsigned int LCD_RS;
static unsigned int LCD_RW;


/* Control output lines
 * Write to baseaddress+2
 */
#define nSTRB   0x01    /* pin 1; negative logic */
#define STRB    0x01
#define nLF     0x02    /* pin 14 */
#define LF      0x02
#define INIT    0x04    /* pin 16; the only positive logic output line */
#define nSEL    0x08    /* pin 17 */
#define SEL     0x08
#define ENIRQ   0x10    /* Enable IRQ via ACK line (don't enable this without
                         * setting up interrupt stuff too) */
#define ENBI    0x20    /* Enable bi-directional port (is nice to play with!
                         * I first didn't know a SPP could do this) */

#define OUTMASK 0x0B    /* SEL, LF and STRB are hardware inverted */
                        /* Use this mask only for the control output lines */
                        /* XOR with this mask ( ^ OUTMASK ) */


/*
 * LCD pins and functions
 */
/* control bits */
#if 0
#ifdef old
#define LCD_RS		LF
#define LCD_RW		0x00
#else
#define LCD_RS		INIT /*0xf8*/ /* INIT, PIN16, inverted */
#define LCD_RW		LF
#endif
#endif
#define LCD_EN		STRB

/* functions (RS==1) */
#define LCD_CLEAR	0x01
#define LCD_HOME	0x02
#define LCD_MODESET	0x04
#define LCD_ONOFF	0x08
#define LCD_EXTFUNC	0x08	/* RE must be set to 1 using LCD_FUNCSET */
#define LCD_SHIFT	0x10
#define LCD_SCRL_EN	0x10	/* RE == 1 */
#define LCD_FUNCSET	0x20
#define LCD_SETCGRAM	0x40
#define LCD_SETSEGRAM	0x40	/* RE == 1 */
#define LCD_SETDDRAM	0x80
#define LCD_SETSCRL	0x80	/* RE == 1 */



/*
 * Inputs BITS (for keypad)
 */
#define BIT_A	(32)
#define BIT_B	(64)
#define BIT_C	(128)
#define BIT_D	(16)


#define TIME_EVENT	100 /* Check fora key event each 100ms */



/*
 * Setting module defaults
 */
const int LCD_MAJOR=120;

static int LCD_ADDRESS=0x0378;
static int LCD_COLS=20;
static int LCD_LINES=4;
static int TIME_SHORT=40;
static int TIME_LONG=100;
  
static int io=0;
static int cols=0;
static int lines=0;
static int t_short=0;
static int t_long=0;
static int a_type=0;
static int w_mode=0;

#if defined KERNEL21 || defined KERNEL24
MODULE_PARM(io, "i"); 
MODULE_PARM_DESC(io, "base io address)");
MODULE_PARM(cols, "i");
MODULE_PARM_DESC(cols, "Number of columns");
MODULE_PARM(lines, "i");
MODULE_PARM_DESC(lines, "Number of lines");
MODULE_PARM(t_short, "i");
MODULE_PARM(t_long, "i");
MODULE_PARM(a_type, "i");
MODULE_PARM(keys,"i");
MODULE_PARM_DESC(keys, "4 keys keypad conected (1 connected  0 not conected)");
MODULE_PARM(w_mode,"i");
MODULE_PARM_DESC(w_mode, "Wiring mode, 0=old style 1=winamp style");
MODULE_LICENSE("GPL");
#endif


EXPORT_NO_SYMBOLS;



/*
 * Some prototypes
 */
void pos_cursor(unsigned char, unsigned char);


/*
 * Sleep nicely for 'n' uS
 * Maybe this saves some cpu cycles and sysload
 */
void sleep_delay(unsigned long n)
{
	int d=n/(1000000/HZ);

	if (!d)
		udelay(n);
	else {
		/* Yield CPU time */
		unsigned long x=jiffies;
		while((jiffies-x) <= d)
			schedule();
	}
}


/*
 * LCD specific commands
 */

/*
 * Set to instruction mode and write a command byte
 */
void write_command(char command)
{
	outb((0x00 ) ^ OUTMASK, LCD_ADDRESS + 2);
	sleep_delay(TIME_SHORT);
	outb((LCD_EN ) ^ OUTMASK, LCD_ADDRESS + 2);
	sleep_delay(TIME_SHORT);
	outb(command, LCD_ADDRESS);  
	sleep_delay(TIME_SHORT);
	outb((0x00) ^ OUTMASK, LCD_ADDRESS + 2);
	sleep_delay(TIME_SHORT);
	if (command == LCD_CLEAR) sleep_delay(2000);	/* 1640 works on mine but others seem slower */
}


/*
 * Set data mode and write a data byte
 */
void write_data(char data)
{
	outb((LCD_RS | LCD_EN) ^ OUTMASK, LCD_ADDRESS + 2);	/* set EN to 1 and RS to 1 */
	sleep_delay(TIME_SHORT);
	/* write the data translated by c_table.h */
	outb(c_table[(unsigned char)data], LCD_ADDRESS);
	sleep_delay(TIME_LONG);
	outb((LCD_RS) ^ OUTMASK, LCD_ADDRESS + 2);	/* clear out EN */
	sleep_delay(TIME_SHORT);
}


/*
 * Resets LCD panel and sets initial mode, i.e.
 * underline cursor, not blinking
 * one line mode
 * small font
 */
void init_lcd(void)
{
	sleep_delay(15000);
	write_command(LCD_FUNCSET | 0x10);		/* Set 8-bit mode */
	sleep_delay(4200);
	write_command(LCD_FUNCSET | 0x10);		/* Set 8-bit mode */
	sleep_delay(200);
	write_command(LCD_FUNCSET | 0x10);		/* Set 8-bit mode */
	sleep_delay(4200);
	write_command(LCD_FUNCSET | 0x10 | 0x08);	/* Set 8-bit mode, font, matrix */
	sleep_delay(240);
	write_command(LCD_ONOFF);			/* Set display off */
	sleep_delay(240);
	write_command(LCD_ONOFF | 0x04 | 0x02);	/* Set display on, cursor on */
	sleep_delay(1740);
	write_command(LCD_MODESET | 0x02);		/* Set entry mode, cursor moves right, shift display */
	if ((a_type == 1) && (LCD_LINES > 2)) {
		write_command(LCD_FUNCSET | 0x10 | 0x08 | 0x04);	/* enter ext-func-set */
		sleep_delay(240);
		write_command(LCD_EXTFUNC | 0x01);		/* Set 4-line mode */
		sleep_delay(240);
		write_command(LCD_FUNCSET | 0x10 | 0x08);	/* leave ext-func-set */
		sleep_delay(240);
	}
	sleep_delay(240);
	write_command(LCD_CLEAR);
}


void init_cgram(void)
{
unsigned int a,b;

	write_command(LCD_SETCGRAM);		/* Move to first CG-RAM address */
	/* We have 8x8 bytes for our own chars */
	for (a=0; a<8; a++) {
		for (b=0; b<8; b++) {
			write_data(0);		/* At init we clear the CG-RAM */
		}
	}
	pos_cursor(dcol,dline);
}


void wr_cgram(char which, char *buf)
{
unsigned int a;

	if ((which < 0) || (which > 7))
		return;
	write_command(LCD_SETCGRAM + (8 * which));	/* Move to CG-RAM address (char# * 8) */
	/* We have 8x8 bytes for our own chars */
	for (a = 0; a < 8; a++) {
		write_data(buf[a]);		/* Write the 8 char-defining bytes */
	}
	sleep_delay(200);
	pos_cursor(dcol,dline);			/* Bring us back to DD-RAM */
}


/*
 * Following functions are for tty like handling of the panel
 * i.e. set cursor position, clear display, home cursor, etc.
 */

/*
 * Sets cursor to certain position
 * col  = column [0-19]
 * line = line [0-3]
 */
void pos_cursor(unsigned char col, unsigned char line)
{
unsigned char offset=0;
unsigned char pos=0;

	/* some sanity checks */
	if (col > (LCD_COLS-1) ) {
		col=0;
		line++;
	}
	if (line > (LCD_LINES-1) ) line = 0;

	/* line address offsets (weird, aren't they?) */
	if (a_type==0) {
		switch (line) {
			case 0:
				offset=0;
				break;
			case 1:
				offset=64;
				break;
			case 2:
				offset=20;
				break;
			case 3:
				offset=84;
				break;
		}
	} else {
		switch (line) {
			case 0:
				offset=0;
				break;
			case 1:
				offset=0x20;
				break;
			case 2:
				offset=0x40;
				break;
			case 3:
				offset=0x60;
				break;
		}
	}

	/* Command for addressing is LCD_SETDDRAM so we add all this here */
	pos = LCD_SETDDRAM + offset + col;
	/* Update our private position counter */
	dline = line;
	dcol = col;
	/* now write it out */
	write_command(pos);
}


/*
 * All functions after here are for device driver specific purposes
 */

/*
 * Called when the device is open()ed
 */
#ifdef KERNEL20
static int lcd_open(struct inode *minode, struct file *file)
#endif
#if defined KERNEL21 || defined KERNEL24
static int lcd_open(struct inode *minode, struct file *mfile)
#endif
{
	if (lcd_usage != 0)
		return -EBUSY;

	MOD_INC_USE_COUNT;
	lcd_usage = 1;
#ifdef DEBUG
	printk(KERN_DEBUG "LCD opened\n");
#endif

return(0);
}


/*
 * Called when the device is close()ed
 */
#ifdef KERNEL20
static void lcd_release(struct inode *minode, struct file *mfile)
#endif
#if defined KERNEL21 || defined KERNEL24
int lcd_release(struct inode *minode, struct file *mfile)
#endif
{
	MOD_DEC_USE_COUNT;
	lcd_usage=0;
#ifdef DEBUG
	printk(KERN_DEBUG "LCD module closed\n");
#endif

#if defined KERNEL21 || defined KERNEL24
return 0;
#endif
}


/*
 * Called if data is to be written out to the device
 */
#ifdef KERNEL20
int lcd_write_byte(struct inode *minode, struct file *mfile, const char *gdata, int length)
#endif
#if defined KERNEL21 || defined KERNEL24
ssize_t lcd_write_byte(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
#endif
{
int i,cnt;
static const char *data;
char c,cgenbuf[8];

	data=gdata;

	c=0;
	for (i=0; i<length; i++) {
#ifdef KERNEL20
		c=get_user((char *)(data+i));
#endif
#if defined KERNEL21 || defined KERNEL24
		get_user(c,data+i);
#endif
#ifdef DEBUG
		printk(KERN_DEBUG "LCD: starting %d\n",i);
		printk(KERN_DEBUG "LCD: which is '%c'\n",c);
#endif
		if (c==10) { /* line feed */
			dline++;
			dcol=0;
			pos_cursor(dcol,dline);
		} else
		if (c==27) {
			if ((i+1)<length) {
				i+=1;
#ifdef KERNEL20
				c=get_user((char *)(data+i));
#endif
#if defined KERNEL21 || defined KERNEL24
				get_user(c,(char *)(data+i));
#endif
				if (c=='c') {
					/* Clear display */
					write_command(LCD_CLEAR);
					dcol=0;
					dline=0;
				} else
				if (c=='h') {
					/* Cursor home */
					write_command(LCD_SETDDRAM);
					dcol=0;
					dline=0;
				} else
				if (c=='K') {
					/* Cursor off */
					write_command(LCD_ONOFF | 0x04);
				} else
				if ((c=='k')||(c=='B')) { 
					/* Cursor on and blink off */
					write_command(LCD_ONOFF | 0x04 | 0x02);
				} else
				if (c=='b') { 
					/* Cursor and blink on */
					write_command(LCD_ONOFF | 0x04 | 0x02 | 0x01);
				} else
				if (c=='g') {
					/* Goto xy by Ulf Lanz */
					if((i+2) < length) {
#ifdef KERNEL20
						i+=1;
						new_dcol=get_user((char *)(data+i));
						i+=1;					
						new_dline=get_user((char *)(data+i));
#endif
#if defined KERNEL21 || defined KERNEL24
                                                i+=1;
                                                get_user(new_dcol,(char *)(data+i));
                                                i+=1;
                                                get_user(new_dline,(char *)(data+i)); 
#endif
					dcol=new_dcol;
					dline=new_dline;
					pos_cursor(dcol,dline);

					}
				} else
				if (c=='R') {
					/* Reset display */
					init_lcd();
					dcol=0;
					dline=0;
				} else
				if (c<8) {
#ifdef DEBUG
					printk(KERN_DEBUG "Def.char\n");
#endif
					if ((i+8)<length) {
						for (cnt=0; cnt<8; cnt++) {
#ifdef KERNEL20
							cgenbuf[cnt]=get_user((char *)(data+i+cnt+1));
#endif
#if defined KERNEL21 || defined KERNEL24
							get_user(cgenbuf[cnt],(char *)(data+i+cnt+1));
#endif
						};
						wr_cgram(c,cgenbuf);
						i+=8;
					}
#ifdef DEBUG
					else
						printk(KERN_DEBUG "Def. too short\n");
#endif
				}
			}
		} else
		if (c==13) { /* carriage return */
			dcol=0;
			pos_cursor(dcol,dline);
		} else {
			write_data(c);
			dcol++;
			if (dcol>(LCD_COLS-1)) {
				dcol=0;
				dline++;
				pos_cursor(dcol,dline);
			}
		}
	}
return(length);
}


/* 
 *	Return the actual status of the keypad
 *	only a byte:
 * 		7 6 5 4 3 2 1 0	Bits
 *		x x x x D C B A Buttons
 */
__u8 read_keys(void)
{
		char button_A,button_B,button_C,button_D;
		__u8 input;
		__u8 button_status;

		/* Read the control byte */
		input = inb(LCD_ADDRESS + 1);

		/* the input bit of C button is inverted */
		input = input ^ BIT_C;

		/* get the buttons status */
		button_A = !(input & BIT_A);
		button_B = !(input & BIT_B);
		button_C = !(input & BIT_C);
		button_D = !(input & BIT_D);

		/* compose the data byte 
		   7 6 5 4 3 2 1 0
		   x x x x D C B A
		*/
		button_status = (button_A * 1) + (button_B * 2) + (button_C * 4) + (button_D * 8);				
		
		return button_status;
}




/*
 * Called if data is to be read
 */
#ifdef KERNEL20
int lcd_read_byte(struct inode *inode, struct file *mfile, char *data, int length)
#endif
#if defined KERNEL21 || defined KERNEL24
ssize_t lcd_read_byte(struct file *inode, char *data, size_t length, loff_t *loff_what)
#endif
{
		if ( ! keys )
		{
				/* If there isn`t a keypad we can`t read from the device */
#ifdef DEBUG
				printk(KERN_DEBUG "LCD: Trying to read readonly device (No keypad) \n");
#endif
				return(EACCES);
		}else{
				__u8 button_status;
				

				/* Ok, we have a keypad, so we read the keys */
				button_status = read_keys();

#ifdef DEBUG
				printk(KERN_DEBUG "LCD: Keys 0x%02x\n", button_status);
#endif	
				/* We supose that at least the user want to read one byte,
				   so we return the status byte */
				put_user(button_status,data);				
				/* only a byte */
				return 1;
		}
}



#if defined KERNEL24 || defined KERNEL21			/* I don`t know how to implement the ioctls at 2.0.x series :-( */

/* 
 * implementation of the ioctls interface
 */
int lcd_ioctl(struct inode * inode, struct file * file, unsigned int ioctl_num, unsigned long ioctl_param)
{
		__u8 cmd = 0;
		__u8 keys_status, new_keys_status, keys_change;
		int i;
		int found;
		__u8 event;
		

		switch ( ioctl_num ){
		case IOC_WAIT_KEY:
#ifdef DEBUG
				printk(KERN_DEBUG "LCD: ictl IOC_WAIT_KEY\n");
#endif			
				if ( ! keys ){
						/* If there isn`t a keypad we can`t wait forkey event */
#ifdef DEBUG
						printk(KERN_DEBUG "LCD: ioctl IOC_WAIT_KEY without a keypad \n");
#endif
						return -EINVAL;
				}else{
						/* We have a keypad */

						/* get the actual status */
						keys_status = read_keys();

						/* wait for diferent status */
						found = 0;
						do
						{	
								new_keys_status = read_keys();
								if ( new_keys_status != keys_status ){
										/* We found an event */
										found  = 1;
								}
								if ( !found ){
										current->state = TASK_INTERRUPTIBLE;
										schedule_timeout(MSECS_TO_JIFFIES(TIME_EVENT));
										current->state = TASK_RUNNING;
										if(signal_pending(current)){
												return -ERESTARTSYS;
										}
								}
						}while ( !found );
						/* We have a key event (new_keys_status != keys_status) */
						/* Check what key has changed, from A to C, only detect the first change */
						keys_change = keys_status ^ new_keys_status;
						printk(KERN_DEBUG "LCD: ioctl IOC_WAIT_KEY change 0x%02x\n",keys_change);
						for ( i = 0 ; i<4  ; i++ ,keys_change >>= 1){
								/* test the first bit */
								if ( (keys_change & 0x01) ){
										break;
								}
						}
						/* Check if the key was pressed 0->1 or released 1->0 */
						event = 0;
						event = (new_keys_status >> i)<<4;
						/* the event byte:
						   bit 4 = 1 pressed
						       4 = 0 released
						   bit0 - bit3 number of the key A -> 0, B-> 1,...
						*/
						event += i;
						
						/* return the event to the user */
						put_user(event,(unsigned char *)ioctl_param);
				}
				break;
		case IOC_CMD:
			
				/* get the command */
				cmd = (unsigned char) ioctl_param;
#ifdef DEBUG
				printk(KERN_DEBUG "LCD: ictl IOC_CMD command 0x%02x\n", cmd);
#endif			

				switch ( cmd )
				{
				case 'c':
								/* Clear display */
						write_command(LCD_CLEAR);
						dcol=0;
						dline=0;
						break;
				case 'h':
								/* Cursor home */
						write_command(LCD_SETDDRAM);
						dcol=0;
						dline=0;
						break;
				case 'K':
								/* Cursor off */
						write_command(LCD_ONOFF | 0x04);
				case 'k':
				case 'B':
								/* Cursor on and blink off */
						write_command(LCD_ONOFF | 0x04 | 0x02);
						break;
				case 'b':
								/* Cursor and blink on */
						write_command(LCD_ONOFF | 0x04 | 0x02 | 0x01);
						break;
				case 'R':
								/* Reset display */
						init_lcd();
						dcol=0;
						dline=0;
						break;

				default:
#ifdef DEBUG
						printk(KERN_DEBUG "LCD: ictl IOC_CMD unknow command 0x%02x\n", cmd);
#endif			
						return -EINVAL;
						break;
				}			
		}
		return 0;
}
#endif

/*
 * Called when module is unloaded
 */
void cleanup_module(void)
{
	release_region(LCD_ADDRESS,3);
	printk(KERN_INFO "LCD: I/O region freed\n");
	if (unregister_chrdev(LCD_MAJOR,"LCD panel"))
		printk(KERN_ERR "LCD: module cleanup failed\n");
	else
		printk(KERN_INFO "LCD: module cleanup succeeded\n");
}


struct file_operations lcd_fops=
{
#ifdef KERNEL24
	owner:		THIS_MODULE,
#endif
	read:		lcd_read_byte,
	write:		lcd_write_byte,
#if defined KERNEL24 || defined KERNEL21
	ioctl:		lcd_ioctl,
#endif
	open:		lcd_open,
	release:	lcd_release,
};

/*
 * First function called when module is inserted
 */
int init_module(void)
{
	printk(KERN_INFO "LCD device %s init:\n",LCD_MODULE_VERSION);

	/* Check IO-Address */
	if (io!=0)
		LCD_ADDRESS = io ; 

	/* Check for lines */
	if ( (lines==1) || (lines==2) || (lines==4) )
		LCD_LINES=lines;
 
	/* Check for cols */
	if ((cols==16) || (cols==20) || (cols==24) || (cols==32) || (cols==40))
		LCD_COLS=cols;
 
	/* Check for t_short */
	if (t_short!=0)
		TIME_SHORT=t_short ; 

	/* Check for t_short */
	if (t_long!=0)
		TIME_LONG=t_long ; 

	if (register_chrdev(LCD_MAJOR,"LCD panel",&lcd_fops))
		printk(KERN_ERR "unable to get major 120 for LCD device\n");
	else {
		if (!check_region(LCD_ADDRESS,3)) {
			request_region(LCD_ADDRESS,3,"LCD panel");
			printk(KERN_INFO "  got 3 addresses from %#x\n",LCD_ADDRESS);
			printk(KERN_INFO "  Display is %d Lines with %d Cols\n",LCD_LINES,LCD_COLS);
			printk(KERN_INFO "  Timing is Short: %d, Long: %d\n",TIME_SHORT,TIME_LONG);
			printk(KERN_INFO "  Keypad: %s\n",keys?"connected":"no connected");
			if (w_mode) {
				LCD_RS=INIT;
				LCD_RW=LF;
				printk(KERN_INFO "  Using winamp style wiring\n");
			} else {
				LCD_RS=LF;
				LCD_RW=0x00;
				printk(KERN_INFO "  Usign old style wiring\n");
			}
			lcd_usage = 0;
			init_lcd();
			init_cgram();
			dline = 0;
			dcol = 0;
		} else {
			release_region(LCD_ADDRESS,3);
			unregister_chrdev(LCD_MAJOR,"LCD panel");
			printk(KERN_ERR "  could not get 3 addresses from %#x\n",LCD_ADDRESS);
		}
	}

return 0;
}
