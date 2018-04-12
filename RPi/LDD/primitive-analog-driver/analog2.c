/** @file analog2.c
 * @author Steven Clark (davolfman@gmail.com clark214@cougars.csusm.edu)
 * @brief Software soundcard through DAC on the GPIO pins of a Raspberry Pi
 * Source: https://hackaday.io/project/10890-devanalog0
 */

/* General kernel module include */
#include <linux/kernel.h>
/* include for init/exit functions and their dyanmic loading */
#include <linux/init.h>
/* general driver infrastructure*/
#include <linux/module.h>
/* provides the ability to link a simple character device into the modern sysfs structure, also links into udev to provide device node*/
#include <linux/miscdevice.h>
/* provides copy_from_user for importing data from the user address space*/
#include <linux/uaccess.h>
/* needed for file operations*/
#include <linux/fs.h>
/* header file provides addresses of SOC registers and conversion functions*/
#include <mach/platform.h>
/* provides a macro needed for accessing IO register pointers*/
#include <asm/io.h>
/* mutexes are used to prevent simultaneous writes*/
#include <linux/mutex.h>
/* macros for efficient manipulation of power-of-2-sized circular buffers*/
#include <linux/circ_buf.h>
/* high-resolution timer system used to replace timer interrupts*/
#include <linux/hrtimer.h>

/** size of input buffer is 1 megabyte*/
#define buffersize 1048576
/** pin number for Least Significant Bit of parallel IO port*/
#define startpin 10
/** macro defines a ~22050hz frequency for timers*/
#define sampleinterval ktime_set(0,45351)

/**@brief The layout of the GPIO registers of the Raspberry PI
 */
struct gpioRegSet {
	uint32_t GPFSEL[6];/**< GPIO function select, 3 bits per pin, 10 pins per word*/
	uint32_t reserved1;
	uint32_t GPSET[2];/**< GPIO set, 1 bit per pin, 1s written will toggle an output high, 0s will be ignored*/
	uint32_t reserved2;
	uint32_t GPCLR[2];/**< GPIO clear, 1bit per pin, 1s written will toggle an output low, 0s will be ignored*/
	uint32_t reserved3;
	uint32_t GPLEV[2];/**< input level*/
	uint32_t reserved4;
	uint32_t GPEDS[2];/**< input edge detect status*/
	uint32_t reserved5;
	uint32_t GPREN[2];/**< rising edge detect*/
	uint32_t reserved6;
	uint32_t GPFEN[2];/**< falling edge detect*/
	uint32_t reserved7;
	uint32_t GPHEN[2];/**< pin high detect*/
	uint32_t reserved8;
	uint32_t GPLEN[2];/**< pin low detect*/
	uint32_t reserved9;
	uint32_t GPAREN[2];/**< aysnchronous rising edge*/
	uint32_t reserved10;
	uint32_t GPAFEN[2];/**< asynchronous falling edge*/
	uint32_t reserved11;
	uint32_t GPPUD;/**< pull up/down resistor controll*/
	uint32_t GPPUDCLK[2];/**< clock involved in pull up/down setup*/
	uint32_t reserved12;
	/*uint4_t test;*//*unknown function can't declare unaligned bits, left in comments for completeness*/
};

/** @brief pointer to the GPIO pins, in the form of a pointer to our gpioRegSet structure*/
static struct gpioRegSet *bcm2709gpio = (void *) __io_address(GPIO_BASE);

/** @brief structure of our input buffer*/
struct circbuf{
	size_t tail, head; /**< dequeue/read index and enqueue/write index*/
	unsigned char buffer[buffersize];/**< circular buffer backing store*/
};

/** @brief The circular input buffer in use by our program*/
static struct circbuf mybuffer;

/** @brief the mutex used to ensure only one write executes at a time*/
static DEFINE_MUTEX(inputlock);

/** @writes a byte of data to 8 GPIO pins set&clear registers starting with pin firstpin
 * @param sample the byte to write to the pin
 * @param firstpin The starting pin of 8 consecutive GPIOs to write
 */
static void pin_byte(uint8_t sample, unsigned firstpin){
	uint32_t outputmask = 0b11111111 << firstpin;/**< used to mask off GPIO pins we don't want to toggle*/
	uint32_t samplewide = ((uint32_t)sample) << firstpin;/**< position byte within a 32-bit word in the location corresponding to it's GPIO pins*/
	
	bcm2709gpio->GPCLR[0] = (~samplewide) & outputmask;/**< mask our zeros and write them to the clear register*/
	bcm2709gpio->GPSET[0] = samplewide & outputmask;/**< mask our ones and write them to our set register*/
}

/** @brief timer callback function that raises the DAC's WR pin high to write the registers after it has been low long enough
 * @param mytimer the timer that called this callback
 * @return whether to restart the timer after this or not
*/
static enum hrtimer_restart DACrefresh(struct hrtimer* mytimer){
  bcm2709gpio->GPSET[0] = 256 << startpin;/**< WR is attached to the 9th pin of our GPIO port*/
  return HRTIMER_NORESTART;/**< don't repeat this timer without being explicitly started*/
}

/** @brief high-resolution timer used for the write-signal to the DAC*/
static struct hrtimer writeclock;

/**@brief hrtimer callback function for writing data to the DAC through the GPIO pins
 * @param mytimer The timer that called this callback function
 * @return whether to restart the timer or not
 */
static enum hrtimer_restart timedRefresh(struct hrtimer* mytimer) {
	/*static uint32_t sampletest = 0;*//* sampletest is used to write a message once every second to verify the timer is approximately accurate*/
	bcm2709gpio->GPCLR[0] = 256 << startpin;/**< lower the WR pin on the DAC by lowering pin 9 of out GPIO bank*/
	pin_byte(mybuffer.buffer[mybuffer.tail], startpin);/**< write the current sample to the GPIO bank*/
	/*sampletest++;*/
	
	/*if((sampletest % 22050) == 0){
	  printk("ticktock\n");
	}*/
	
	/** if there's more than one sample in the buffer, increment the dequeue index*/
	if(CIRC_CNT(mybuffer.head,mybuffer.tail,buffersize) > 1){
		mybuffer.tail = (mybuffer.tail+1) % buffersize;
	}
	
	/** run a timer to raise the DAC's WR pin a microsecond from now*/
	hrtimer_start(&writeclock, ktime_set(0,1000), HRTIMER_MODE_REL);
	
	/** add one sample interval to our clock*/
	hrtimer_forward_now(mytimer, sampleinterval);
	
	/** reset the timer to trigger again later*/
	return HRTIMER_RESTART;
}
/** @brief hrtimer used to run the GPIO output cycle*/
static struct hrtimer refreshclock;

/** @brief takes data written to our device file and writes as much as will fit into the buffer
 * @param filp a file structure pointer, ignored
 * @param buf pointer to the data written in user space
 * @param count the amount of data the user wants to write
 * @param f_pos a file postion offset ignored as nonsense in this case
 * @return the number of bites written
 */
static ssize_t writeop(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){
	size_t pulltemp;/**< stores temporary conservative copy of dequeue index of the circular buffer*/
	size_t pushtemp;/**< stores temporary conservative copy of enqueue index of the circular buffer*/
	size_t space;/**< space available in buffer*/
	size_t firstspace;/**< space available in the buffer before wrapping*/
	size_t secondspace;/**< space available in the buffer after writing */
	size_t firstwrite;/**< bytes written to the buffer before the wrap*/
	size_t secondwrite;/**< bytes written to the buffer after the wrap*/
	size_t notwritten = 0;/**< bytes not written if a copy_from_user fails*/

	/** block any other write methods that try to start while this one is in progress fail if impossible*/
	if(mutex_lock_interruptible(&inputlock))
		return 0;
	/** set the variables to their values*/
	pulltemp = mybuffer.tail;
	pushtemp = mybuffer.head;
	space = CIRC_SPACE(pushtemp, pulltemp, buffersize);
	firstspace = CIRC_SPACE_TO_END(pushtemp, pulltemp, buffersize);
	secondspace = space - firstspace;
	firstwrite = (count < firstspace) ? count : firstspace;
	secondwrite = (count < space) ? count - firstwrite : secondspace;
	
	/** Write any segments of data into the buffer before or after the wrap at the end of the backing store*/
	if(firstwrite)
		notwritten = copy_from_user((void *)(mybuffer.buffer + pushtemp), (const void __user*)buf, firstwrite);
	/* if there was an error in copy_from_user return only the number of bytes copied*/
	if(notwritten){
		/* increment the enqueue index by the number of bytes actually written*/
		mybuffer.head = (pushtemp + firstwrite - notwritten) % buffersize;
		mutex_unlock(&inputlock);
		return firstwrite-notwritten;
	}
	if(secondwrite)/**< if there's some data to write to the other side of the loop, do so*/
		notwritten = copy_from_user((void *)mybuffer.buffer, (const void __user*)(buf+firstwrite), secondwrite);
	if(notwritten){
		mybuffer.head = (pushtemp + firstwrite + secondwrite - notwritten) % buffersize;
		mutex_unlock(&inputlock);
		return firstwrite+secondwrite-notwritten;
	}
	
	/** increment the enqueue index by the number of bytes actually written*/
	mybuffer.head = (pushtemp + firstwrite + secondwrite) % buffersize;
	/** let other writes commence*/
	mutex_unlock(&inputlock);
	/** return the number of bytes written*/
	return firstwrite+secondwrite;
}

/** @brief always succeed at opening the file
 * @param ignored0 an inode pointer, not used
 * @param ignored1 a file pointer, not used
 * @return success
 */
static int openop(struct inode* ignored0,struct file * ignored1){
	return 0;
}

/** @brief always succeed at releasing the file
 * @param ignored0 an inode pointer, not used
 * @param ignored1 a file pointer, not used
 * @return success
 */
static int releaseop(struct inode* ignored0,struct file * ignored1){
	return 0;
}

/** @brief the file operations supported by this device
 */
static struct file_operations opslist = {
		.owner = THIS_MODULE,/**< the opslist is owned by the present module*/
		.write = &writeop,/**< pointer to the write file operation function*/
		.open = &openop,/**< pointer to the open file operation function*/
		.release = &releaseop,/**< pointer to release write file operation function*/
};

/** @brief information to define our "misc" type of device in sysfs*/
static struct miscdevice ourDev = {
		MISC_DYNAMIC_MINOR,/**< minor number assigned, mostly ignored*/
		"analog0",/**< our device will be called "analog0" for /dev/analog0*/
		&opslist/**< it will use the file ops in opslist*/
};
/** @brief Setup function for our driver
 * @return 0 for success
 */
static int myinit(void){
	int devRegErr;
	
	/** attempt to register our miscelaneous device*/
	devRegErr = misc_register(&ourDev);

	/** if we failed log an error message, and don't continue with setup*/
	if(devRegErr)
		printk(KERN_ERR "Unable to register misc device for analog0\n");
	else{
		/** initialize the ring buffer */
		mybuffer.head = 1;
		mybuffer.tail = 0;
		mybuffer.buffer[0] = 127;
	
		/** set the full bank of 10 pins to output*/
		bcm2709gpio->GPFSEL[1]=0b001001001001001001001001001001;
	
		/** output 127 directly to our GPIO pins*/
		pin_byte(127, startpin);

		/** initialize the DAC write timer and set it's callback but do not start it*/
		hrtimer_init(&writeclock, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		writeclock.function = &DACrefresh;
	
		/** initialize, set callback, and start our DAC loader timer*/
		hrtimer_init(&refreshclock, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		refreshclock.function = &timedRefresh;
		hrtimer_start(&refreshclock, sampleinterval, HRTIMER_MODE_REL);
	}
	/*return error if one was found setting up device*/
	return devRegErr;	
}

/**@brief driver cleanup function
 */
static void myexit(void){
	/** don't try to deregister resources we never registered*/
	if(refreshclock.function == &timedRefresh){
		/** cancel the timers, waiting callbacks to complete if neccesary*/
		hrtimer_cancel(&refreshclock);
		hrtimer_cancel(&writeclock);
		/** deregister our device with the system*/
		misc_deregister(&ourDev);
	}
}

module_init(myinit);/**< set myinit as setup function*/
module_exit(myexit);/**< set myexit as cleanup function*/

MODULE_LICENSE("GPL");/**< set the license to GPL so the kernel doesn't complain about being tainted in errors*/
