// Link : https://www.codeproject.com/Articles/1032794/Simple-I-O-device-driver-for-RaspberryPi

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/fs.h>
#include <asm/uaccess.h>	/* For put_user */
#include <linux/ioport.h>
#include <asm/io.h>

#define SUCCESS 0
#define DEVICE_NAME "io_dev" // Dev name as it appears in /proc/devices   
#define BUF_LEN 80           // Max length of the message from the device 


static unsigned PORT = 0x20200000;
static unsigned RANGE = 0x40;

unsigned cmd_word;
unsigned out_word;

u8 *addr;

static int Major;            /* Major number assigned to our device driver */
static int DeviceOpen = 0;   /* Is device open?
                              * Used to prevent multiple access to device */
static char msg[BUF_LEN];    /* The message the device will give when asked */
static char *pMsg;

/****************************************************************************/
/* Module operations                                                        */
/****************************************************************************/
static int device_open (
    struct inode * inode, 
    struct file *file)
{
    u32 cmd;    // command word to write
    pMsg = msg;

    sprintf(msg,"Not implemented\n");

    if (DeviceOpen)
        return -EBUSY;

    DeviceOpen++;
    try_module_get(THIS_MODULE);
    addr = ioremap(PORT, RANGE);
    if (addr  != NULL)
    {
        cmd = 0;
        writel(cmd, (addr+4)); // clear the setting of pin 10
        cmd = 1;
        writel(cmd, (addr+4)); // set pin 10 as output
        return SUCCESS;
    }
    else
      return -ENODEV;
}

static int device_release(
    struct inode *inode, 
    struct file *file)
{
    writel(0x00, (addr+4));    // clear the setting before release
    DeviceOpen--;              // make ready for the next access
    module_put(THIS_MODULE);

    return 0;
}

static ssize_t device_read(
    struct file *filp,
    char *buffer,	
    size_t length,
    loff_t * offset)
{
    int bytes_read = 1;
    int index;
    u32 res;   // status word to read
    pMsg = msg;
    res = readl(addr+0x34);
    msg[3] = res & 0xFF;
    msg[2] = (res >> 8) & 0xFF;
    msg[1] = (res >> 16) & 0xFF;
    msg[0] = (res >> 24) & 0xFF;
    msg[4] = 0; 
    index = 4;

    while (length && (index >= 0))
    {
        put_user(*(pMsg++), buffer++);
        length--;
        index--;
        bytes_read++;
    }

    return bytes_read;
}

static ssize_t device_write(
    struct file *filp, 
    const char *buff, 
    size_t len, 
    loff_t * off)
{
    int bytes_written = 1;
    u32 cmd;   // command word to write
    if (buff)
    {
        if (buff[0] == 1)
        {
            cmd = 1 << 10;
            writel(cmd, (addr+0x1c));
        }
        else
        {
            if (buff[0] == 0)
            {
                cmd = 1 << 10;
                writel(cmd, (addr+0x28));
            }
        }
    }

    return bytes_written;
}

// Operation structure
static struct file_operations fops = {
	.owner   = THIS_MODULE,
	.open    = device_open,
	.release = device_release,
	.read    = device_read,
	.write   = device_write
};

/****************************************************************************/
/* Module init / cleanup block.                                             */
/****************************************************************************/
void gpio_char_device_init(void)
{
    // TODO
    printk(KERN_NOTICE "Hello\n");
    Major = register_chrdev(0,DEVICE_NAME, &fops);
    if (Major < 0) {
        printk(KERN_ALERT "Registering char device failed with %d\n", Major);
        return Major;
    }
    
    if (check_mem_region(PORT, RANGE) < 0) {
        unregister_chrdev(Major, DEVICE_NAME);
        return -ENODEV;
    }
    else
    {
        if (request_mem_region(PORT, RANGE, DEVICE_NAME) == NULL) {
            unregister_chrdev(Major, DEVICE_NAME);
            return -ENODEV;
        }
        else
        {
            // TODO
        }
    }

    return;
}

void gpio_char_device_cleanup(void)
{
    // TODO
    release_mem_region(PORT, RANGE);
    unregister_chrdev(Major, DEVICE_NAME);
    printk(KERN_NOTICE "Goodbye\n");
}


module_init(gpio_char_device_init);
module_exit(gpio_char_device_cleanup);

/****************************************************************************/
/* Module licensing/description block.                                      */
/****************************************************************************/
MODULE_LICENSE("GPL");
MODULE_AUTHO("Hans-Peter Moser <hanspeter_moser@bluewin.ch>")
MODULE_AUTHOR("Pascal Le Boudin <fxrdude@gmail.com>");
MODULE_DESCRIPTION("Simple GPIO Char Device Driver");
