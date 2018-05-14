/* lcd_module.c
 *
 * LCD driver for HD44780 compatible displays connected to the parallel port.
 *
 * Copyright (C) by Michael McLellan 2000-2002 (mikey@cs.auckland.ac.nz)
 * Released under the terms of the GNU GPL, see file COPYING for more details.
 */

#define LCDMOD_VERSION 0.5.5

#include <linux/version.h>
#ifndef LINUX_VERSION_CODE
#define LINUX_VERSION_CODE 0
#endif
#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0)
#include <asm/segment.h>
#include <linux/sched.h>
#else
#include <asm/uaccess.h>
#endif
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <asm/io.h>

/* defines */
#ifndef DFLT_BASE
#	define DFLT_BASE	0x378	// Default I/O base address of the parallel port
#endif
#ifndef DFLT_DISP_ROWS
#	define DFLT_DISP_ROWS	2	// Default number of rows the display has
#endif
#ifndef DFLT_DISP_COLS
#	define DFLT_DISP_COLS	16	// Default number of columns the display has
#endif
#define MAX_DISP_ROWS	4		// The HD44780 supports up to 4 rows
#define MAX_DISP_COLS	40		// The HD44780 supports up to 40 columns
#define LCD_MAJOR		120     // Major for device file

/* These defines are for the only availble pins on a parallel port
 * which we can use for output besides pins 2 throuh 9 which are used
 * for data. The address for the below pins is IO + 2 eg. 0x378 + 0x02.
 * Some pins on the parallel port are logicaly reversed which is why
 * some which are ON are 0x00.
 */
#define PIN_1_ON		0x00
#define PIN_1_OFF		0x01
#define PIN_14_ON		0x00
#define PIN_14_OFF		0x02
#define PIN_16_ON		0x04
#define PIN_16_OFF		0x00
#define PIN_17_ON		0x00
#define PIN_17_OFF		0x08

#ifdef WIRING_LCDMOD
/* My own wiring.
 *
 * parallel 14		RS (LCD 4)
 * grounded			RW (LCD 5)
 * parallel 1		EN (LCD 6)
 */
#define RW_WRITE		0x00	// Not needing in my wiring.
#define E_ON			PIN_1_ON
#define E_OFF			PIN_1_OFF
#define RS_DATA			PIN_14_ON
#define RS_INST			PIN_14_OFF
#endif

#ifdef WIRING_LCDPROC_8_BIT
/* http://lcdproc.omnipotent.net/
 * wiring from lcdtime aswell?
 *
 * parallel 1		RS (LCD 4)
 * parallel 14		RW (LCD 5)
 * parallel 16		EN (LCD 6)
 */
#define RW_WRITE		PIN_14_OFF
#define RS_DATA			PIN_1_ON
#define RS_INST			PIN_1_OFF
#define E_ON            PIN_16_ON
#define E_OFF           PIN_16_OFF
#endif

#ifdef WIRING_RANDY_RASA
/* http://www.geocities.com/ResearchTriangle/1495/ee_lcd.html
 * http://www.eio.com/lcd2014.htm
 * http://www.rendo.dekooi.nl/~fjbgp/smartie/
 *
 * parallel 16		RS (LCD 4)
 * parallel 14		RW (LCD 5)
 * parallel 1		EN (LCD 6)
 */
#define RW_WRITE		PIN_14_OFF
#define RS_DATA			PIN_16_ON
#define RS_INST			PIN_16_OFF
#define E_ON			PIN_1_ON
#define E_OFF			PIN_1_OFF
#endif

#define TABSTOP			3       // length of tabs

/* input_state states */
#define NORMAL			0
#define ESC				1   	// escape sequence
#define DCA_Y			2   	// direct cursor access, the next input will be the row
#define DCA_X			3   	// direct cursor access, the next input will be the column
#define CGRAM_SELECT	4		// selecting which slot to enter new character
#define CGRAM_ENTRY		5		// adding a new character to the CGRAM
#define CGRAM_GENERATE	6		// waiting fot the 8 bytes which define a character
#define ESC_			10		// waiting for the [ in escape sequence

/* globals */
static int io = DFLT_BASE;
static int disp_rows = DFLT_DISP_ROWS;
static int disp_cols = DFLT_DISP_COLS;
static int quiet = 0;
static unsigned char state[ MAX_DISP_ROWS ][ MAX_DISP_COLS ];	// the current state of the display
static int disp_row = 0, disp_column = 0; 						// current actual cursor position
static int row = 0, column = 0; 								// current virtual cursor position
static int input_state = NORMAL; 								// the current state of the input handler

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,1,0)
MODULE_DESCRIPTION( "LCD parallel port driver for HD44780 compatable controllers" );
MODULE_PARM( io, "i" );
MODULE_PARM( disp_rows, "i" );
MODULE_PARM( disp_cols, "i" );
MODULE_PARM( quiet, "i" );
MODULE_PARM_DESC( io, "Parallel port i/o base address (default: DEFAULT_BASE)" );
MODULE_PARM_DESC( disp_rows, "LCD rows (default: DFLT_DISP_ROWS, max: MAX_DISP_ROWS)" );
MODULE_PARM_DESC( disp_cols, "LCD columns (default: DFLT_DISP_COLS, max: MAX_DISP_COLS)" );
MODULE_PARM_DESC( quiet, "[0,1] wheather to display welcome messages" );
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
//struct proc_dir_entry lcd_proc_entry = { 0, 3, "lcd", S_IFREG | S_IRUGO, 1, 0, 0, 0, NULL, &readProcFile, };
#endif

/* Send an instruction to the display, e.g. move cursor */
static void writeCommand( char command )
{
	/* The E cycle time for a write should be at least 500 ns,
	 * and the minimum E pulse width should be 230 ns.
	 */
	outb( E_ON | RS_INST | RW_WRITE, io + 2 );
	udelay( 250 );
	outb( command, io );		// send command
	outb( E_OFF | RS_INST | RW_WRITE , io + 2 );
	udelay( 250 );
	/* Minimum time to wait after most commands is 39ns except Clear Display
	 * and Return Home which is 1.53ms, we never use Return Home.
	 */
	if( command == 0x01 )
		mdelay( 1.6 );
	else
		udelay( 40 );
}

/* Send character data to the display, i.e. writing to DDRAM */
static void writeData( unsigned char data )
{
	/* lines 2 & 3 are _next_ to lines 0 & 1 */
	//if( row >= 2 )
	//{
	//	row -= 2;
	//	column += 0x14;
	//}

	/* check and see if we really need to write anything */
	if( state[ row ][ column ] != data )
	{
		state[ row ][ column ] = data;
		/* set the cursor position if need be.
		 * Special case for 16x1 displays, They are treated as two
		 * 8 charcter lines side by side, and dont scroll along to
		 * the second line automaticly.
		 */
		if( disp_row != row || disp_column != column ||
				( disp_rows == 1 && disp_cols == 16 && column == 8 ) )
		{
			writeCommand( ( row>=2?(row-2)*0x40:row*0x40 ) | ( row>=2?column+0x14:column ) | 0x80 );
			disp_row = row;
			disp_column = column;
		}

		/* The E cycle time for a write should be at least 500 ns,
		 * and the minimum E pulse width should be 230 ns.
		 */
		outb( E_ON | RS_DATA | RW_WRITE, io + 2 );
		udelay( 250 );
		outb( ( unsigned char ) data, io );
		outb( E_OFF | RS_DATA | RW_WRITE, io + 2 );
		udelay( 250 );
		/* Time to wait after write to RAM is 43ns */
		udelay( 43 );
		disp_column++;
	}
	if ( column < disp_cols - 1 )
		column++;
}

/* Write an entire (5x8) character to the CGRAM,
 * takes the CGRAM index, and a char[ 8 ] binary bitmap.
 */
static void writeCGRAM( int index, unsigned char pixels[] )
{
	int i;

	/* Move address pointer to index in CGRAM */
	writeCommand( 0x40 + ( 8 * index ) );
	for( i = 0; i < 8; i++ )
	{
		/* Write a line of a character. The E cycle time for
		 * a write should be at least 500 ns, and the minimum
		 * E pulse width should be 230 ns.
		 */
		outb( E_ON | RS_DATA | RW_WRITE, io + 2 );
		udelay( 250 );
		outb( ( unsigned char ) pixels[ i ], io );
		outb( E_OFF | RS_DATA | RW_WRITE, io + 2 );
		udelay( 250 );
		/* Time to wait after write to RAM is 43 ns */
		udelay( 45 );
	}
	row = column = -1;
}

static void initDisplay()
{
	char *init_mesage = "lcdmod v0.5.5";
	char *ptr;
	unsigned char volume1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f };
	unsigned char volume2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f };
	unsigned char volume3[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f };
	unsigned char volume4[] = { 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f };
	unsigned char volume5[] = { 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
	unsigned char volume6[] = { 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
	unsigned char volume7[] = { 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
	unsigned char s[]		= { 0x1f, 0x1f, 0x11, 0x0f, 0x11, 0x1e, 0x01, 0x1f };

	/* initialize state array */
	memset( state, ' ', sizeof( state ) );

	/* initialize display */
	writeCommand( 0x38 );    // Function set
	writeCommand( 0x0c );    // Display on
	writeCommand( 0x01 );    // Clear display
	writeCommand( 0x06 );    // Entry mode set
	/* Set the CGRAM to default values */
	writeCGRAM( 0, s );
	writeCGRAM( 1, volume1 );
	writeCGRAM( 2, volume2 );
	writeCGRAM( 3, volume3 );
	writeCGRAM( 4, volume4 );
	writeCGRAM( 5, volume5 );
	writeCGRAM( 6, volume6 );
	writeCGRAM( 7, volume7 );

	if( !quiet )
	{
		/* write a welcome message on the screen */
		row = 0;
		column = ( disp_cols / 2 ) - ( strlen( init_mesage ) / 2 );
		ptr = init_mesage;
		while( *ptr )
		{
			writeData( *ptr );
			ptr = ptr + 1;
		}
	}
}

/* Do a simple read-write test to make sure the port exists. */
static int portExists( int IObase )
{
	outb( 1, IObase );
	if( inb( IObase ) != 1 )
		return 0;
	outb( 0, IObase );
	if( inb( IObase ) != 0 )
		return 0;
	return 1;
}

static void handleInput( unsigned char input )
{
	static int cgram_index = 0;
	static int cgram_row_count;
	static unsigned char cgram_pixels[ 8 ];
	int eof = 0,
		i,
		j,
		temp;

	if ( input_state == NORMAL )
	{
		switch ( input )
		{
			case 0x04: 	// Control-D or EOF
				eof = 1;
				break;
			case 0x08: 	// Backspace
				if ( column > 0 )
				{
					column--;
					writeData( ' ' );
					column--;
				}
				break;
			case 0x09: 	// Tabstop
				column = ( ( ( column + 1 ) / TABSTOP ) * TABSTOP ) + TABSTOP - 1;
				break;
			case 0x0a: 	// Newline
				if ( row < disp_rows - 1 )
					row++;
				else
				{
					/* scroll up */
					temp = column;
					for ( i = 0; i < disp_rows - 1; i++ )
					{
						row = i;
						for( j = 0; j < disp_cols; j++ )
						{
							column = j;
							writeData( state[ i + 1 ][ j ] );
						}
					}
					row = disp_rows - 1;
					column = 0;
					for ( i = 0; i < disp_cols; i++ )
					{
						writeData( ' ' );
					}
					column = temp;
				}
				break;
			case 0x0d: 	// Carrage return
				column = 0;
				break;
			case 0x1b: 	// esc ie. start of escape sequence
				input_state = ESC_;
				break;
			default:
				writeData( input );
		}
	}
	else if ( input_state == ESC_ )
	{
		input_state = ESC;
	}
	else if ( input_state == ESC )
	{
		if( input <= '7' && input >= '0' )
		{
			/* Chararacter from CGRAM */
			writeData( input - 0x30 );
		} else {
			switch ( input )
			{
				case 'A': 		// Cursor up
					if ( row > 0 )
						row--;
					break;
				case 'B': 		// Cursor down
					if ( row < disp_rows - 1 )
						row++;
					break;
				case 'C': 		// Cursor Right
					if ( column < disp_cols - 1 )
						column++;
					break;
				case 'D': 		// Cursor Left
					if ( column > 0 )
						column--;
					break;
				case 'H': 		// Cursor home
					row = 0;
					column = 0;
					break;
				case 'J': 		// Clear screen, cursor doesn't move
					memset( state, ' ', sizeof( state ) );
					writeCommand( 0x01 );
					break;
				case 'K': 		// Erase to end of line, cursor doesn't move
					temp = column;
					for ( i = column; i < disp_cols; i++ )
						writeData( ' ' );
					column = temp;
					break;
				case 'Y': 		// Direct cursor access
					input_state = DCA_Y;
					break;
				case 'R':		// CGRAM select
					input_state = CGRAM_SELECT;
					break;
				default:
					printk( "LCD: unrecognized escape sequence: %#x ('%c')\n", input, input );
			}
		}
		if ( input_state != DCA_Y && input_state != CGRAM_SELECT )
			input_state = NORMAL;
	}
	else if ( input_state == DCA_Y )
	{
		if ( input - 0x1f < disp_rows )
			row = input - 0x1f;
		else
		{
			printk( "LCD: tried to set cursor to off screen location\n" );
			row = disp_rows - 1;
		}
		input_state = DCA_X;
	}
	else if ( input_state == DCA_X )
	{
		if ( input - 0x1f < disp_cols )
			column = input - 0x1f;
		else
		{
			printk( "LCD: tried to set cursor to off screen location\n" );
			column = disp_cols - 1;
		}
		input_state = NORMAL;
	}
	else if ( input_state == CGRAM_SELECT )
	{
		if( input > '7' || input < '0' )
		{
			printk( "LCD: Bad CGRAM index %c\n", input );
			input_state = NORMAL;
		} else {
			cgram_index = input - 0x30;
			cgram_row_count = 0;
			input_state = CGRAM_GENERATE;
		}
	}
	else if( input_state == CGRAM_GENERATE )
	{
		cgram_pixels[ cgram_row_count++ ] = input;
		if( cgram_row_count == 8 )
		{
			writeCGRAM( cgram_index, cgram_pixels );
			input_state = NORMAL;
		}
	}
}

/* Handle device open */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0)
static int lcd_open( struct inode *minode, struct file *file )
#else
int lcd_open( struct inode *minode, struct file *mfile )
#endif
{
	if ( MOD_IN_USE )
		return -EBUSY;

	MOD_INC_USE_COUNT;
	return ( 0 );
}

/* Handle device file close */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0)
static void lcd_release( struct inode *minode, struct file *mfile )
#else
int lcd_release( struct inode *minode, struct file *mfile )
#endif
{
	MOD_DEC_USE_COUNT;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	return 0;
#endif
}

/* Handle write to device file */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0)
int lcd_write_byte( struct inode *minode, struct file *mfile, const char *gdata, int length )
#else
ssize_t lcd_write_byte( struct file *inode, const char *gdata, size_t length, loff_t *off_what )
#endif
{
	int i;
	for ( i = 0; i < length; i++ )
		handleInput( gdata[ i ] );
	return ( length );
}

/* Handle read from device file */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0)
int lcd_read_byte( struct inode *inode, struct file *mfile, char *data, int length )
#else
ssize_t lcd_read_byte( struct file *inode, char *udata, size_t length, loff_t *loff_what )
#endif
{
	return ( EACCES );
}

/* Handle read from proc file */
int readProcFile( char *buffer, char **start, off_t offset, int size, int *eof, void *data )

{
	char *temp = buffer;
	int i, j;

	/* Print module configuration */
	temp += sprintf( temp, "I/O base:        %#x\n"
			"Display rows:    %d\n"
			"Display columns: %d\n\n",
			io, disp_rows, disp_cols );

	/* Print display state */
	temp += sprintf( temp, "+" );
	for( i = 0; i < disp_cols; i++ )
		temp += sprintf( temp, "-" );
	temp += sprintf( temp, "+\n" );

	for( i = 0; i < disp_rows; i++ )
	{
		temp += sprintf( temp, "|" );
		for( j = 0; j < disp_cols ; j++ )
			temp += sprintf( temp, "%c", (state[ i ][ j ] < 10)?'·':state[ i ][ j ] );
		temp += sprintf( temp, "|\n" );
	}
	
	temp += sprintf( temp, "+" );
	for( i = 0; i < disp_cols; i++ )
		temp += sprintf( temp, "-" );
	temp += sprintf( temp, "+\n" );
	return temp - buffer;
}

/* Module cleanup */
void cleanup_module()
{
	char *goodbye_message = "lcdmod removed";
	char *ptr;

	row = 0;
	column = ( disp_cols / 2 ) - ( strlen( goodbye_message ) / 2 );
	ptr = goodbye_message;
	memset( state, ' ', sizeof( state ) );
	writeCommand( 0x01 );
	if( !quiet )
	{
		while( *ptr )
		{
			writeData( *ptr );
			ptr = ptr + 1;
		}
	}
	release_region( io, 3 );
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	remove_proc_entry( "lcd", 0 );
#endif
	if ( unregister_chrdev( LCD_MAJOR, "LCD panel" ) )
		printk( "LCD: module cleanup failed\n" );
	else
		printk( "LCD: module cleanup OK\n" );
}

/* Module initilisation */
int init_module()
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0)
	static struct file_operations lcd_fops =
	    { NULL, lcd_read_byte, lcd_write_byte, NULL, NULL, NULL, NULL, lcd_open, lcd_release, };
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
	static struct file_operations lcd_fops =
	    { NULL, lcd_read_byte, lcd_write_byte, NULL, NULL, NULL, NULL, lcd_open, NULL, lcd_release, };
#else
	static struct file_operations lcd_fops =
	    { NULL, NULL, lcd_read_byte, lcd_write_byte, NULL, NULL, NULL, NULL, lcd_open, NULL, lcd_release, };
#endif

	/* Make sure user didn't pass silly numbers, MAX_DISP_???? are just
	 * arbitary numbers and can be increased if need be.
	 */
	disp_rows = disp_rows<=MAX_DISP_ROWS?disp_rows:MAX_DISP_ROWS;
	disp_cols = disp_cols<=MAX_DISP_COLS?disp_cols:MAX_DISP_COLS;

	if ( register_chrdev( LCD_MAJOR, "LCD panel", &lcd_fops ) )
	{
		printk( "LCD: Error! Unable to bind to major %d\n", LCD_MAJOR );
		return -1;
	} else {
		if( !portExists( io ) )
			printk( "LCD: Warning! Couldn't find parallel port at %#x, proceeding anyway\n", io );

		if ( !check_region( io, 3 ) )
		{
			request_region( io, 3, "LCD panel" );
			initDisplay();
		}
		else
		{
			unregister_chrdev( LCD_MAJOR, "LCD panel" );
			printk( "LCD: Error! Could not get region %#x\n", io );
			return -1;
		}
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
 	if( !create_proc_read_entry( "lcd", 0, 0, readProcFile, NULL ) )
	{
        printk( KERN_ERR "LCD: Error! Can't create /proc/lcd\n" );
        return -ENOMEM;
    }
#endif
	printk( "LCD: init OK, iobase: %#x, rows: %d, columns: %d\n", io, disp_rows, disp_cols );
	return 0;
}
