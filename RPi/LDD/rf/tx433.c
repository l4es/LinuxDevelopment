/* Copyright (C) 2013, Jack Whitham
 * Copyright (C) 2009-2010, University of York
 * Copyright (C) 2004-2006, Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */




#include <linux/platform_device.h>
#include <linux/compat.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/time.h>

#include <asm/io.h>
#include <asm/delay.h>
#include <asm/atomic.h>

#include <linux/types.h>

#define DEV_NAME            "tx433"


#define TX_PIN              24

#define BCM2708_PERI_BASE   0x20000000
#define GPIO_BASE           (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

static void __iomem *gpio;
static uintptr_t gpio1;

static inline void INP_GPIO(unsigned g)
{
    // #define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
    uintptr_t addr = (4 * (g / 10)) + gpio1;
    iowrite32(ioread32(addr) & ~(7<<(((g)%10)*3)), addr);
}

static inline void OUT_GPIO(unsigned g)
{
    // #define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
    uintptr_t addr = (4 * (g / 10)) + gpio1;
    iowrite32(ioread32(addr) | (1<<(((g)%10)*3)), addr);
}

static inline void GPIO_SET(unsigned g)
{
    // sets   bits which are 1 ignores bits which are 0
    // #define GPIO_SET *(gpio+7)  
    iowrite32(g, gpio1 + (4 * 7));
}

static inline void GPIO_CLR(unsigned g)
{
    // clears bits which are 1 ignores bits which are 0
    // #define GPIO_CLR *(gpio+10) 
    iowrite32(g, gpio1 + (4 * 10));
}

static unsigned await_timer = 0;
static struct timeval initial;

static inline unsigned micros(void)
{
    struct timeval t;
    do_gettimeofday(&t);
    t.tv_sec -= initial.tv_sec;
    return ((unsigned) t.tv_sec * (unsigned) 1000000) + t.tv_usec;
}

static inline void await(unsigned us)
{
    await_timer += us;
    while(micros() < await_timer) {}
}

static inline void send_high(void)
{
    GPIO_SET(1 << TX_PIN);
    await(220);
    GPIO_CLR(1 << TX_PIN);
}

static inline void send_zero(void)
{
    send_high(); 
    await(1330);
}

static inline void send_one(void)
{
    send_high(); 
    await(320);
}

static unsigned transmit_code(unsigned tx_code, unsigned attempts)
{
    unsigned i, j, start, stop;

    do_gettimeofday(&initial);
    await_timer = start = micros();
    for (i = 0; i < attempts; i++) {
        send_high(); // Start code
        await(2700);
        j = 32;
        while (j > 0) {
            unsigned bits;
            j -= 2;
            bits = (tx_code >> (unsigned) j) & 3;
            if (bits == 0) {
                send_one(); send_zero();
                send_one(); send_zero();
            } else if (bits == 1) {
                send_one(); send_zero();
                send_zero(); send_one();
            } else if (bits == 2) {
                send_zero(); send_one();
                send_one(); send_zero();
            } else {
                send_zero(); send_one();
                send_zero(); send_one();
            }
        }
        send_high(); // End code
        await(10270); // Gap
    }
    stop = micros();
    return stop - start;
}

static int tx433_open(struct inode *inode, struct file *file)
{
    return nonseekable_open(inode, file);
}

static int tx433_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t tx433_write(struct file *file, const char __user *buf,
                size_t count, loff_t *pos)
{
	long val = 0;
    unsigned code = 0;
    unsigned timing = 0;
    char tmp[9];
    unsigned long flags;

    if (count < 8) {
        return -EINVAL;
    }

    if (copy_from_user(tmp, buf, 8)) {
        return -EFAULT;
    }
    tmp[8] = '\0';
	if (kstrtol(tmp, 16, &val) != 0) {
		return -EINVAL;
    }
    code = val;
    if ((code == 0) || (code >= (1 << 28))) {
		return -EINVAL;
    }

    // ready for transmission
    local_irq_save(flags);
    timing = transmit_code(code, 10);
    local_irq_restore(flags);
    printk(KERN_ERR DEV_NAME ": send %08x took %u\n", code, timing);
    return count;
}

static ssize_t tx433_read(struct file *file, char __user *buf,
                size_t count, loff_t *pos)
{
    return -EINVAL;
}

static struct file_operations tx433_fops = {
    .owner = THIS_MODULE,
    .open = tx433_open,
    .read = tx433_read,
    .write = tx433_write,
    .release = tx433_release,
};

static struct miscdevice tx433_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEV_NAME,
    .fops = &tx433_fops,
};

static int __init tx433_init(void)
{
    unsigned long flags;

    printk(KERN_ERR DEV_NAME ": tx433_init\n");

    gpio = ioremap_nocache(GPIO_BASE, BLOCK_SIZE);
    if (gpio == NULL) {
        printk(KERN_ERR DEV_NAME ": ioremap_nocache failed\n");
        return -ENXIO;
    }
    gpio1 = (uintptr_t) gpio;
    printk(KERN_ERR DEV_NAME ": your number is %p\n", gpio);
    misc_register(&tx433_misc_device);
    local_irq_save(flags);
    INP_GPIO(TX_PIN);
    OUT_GPIO(TX_PIN);
    local_irq_restore(flags);
	return 0;
}

static void __exit tx433_exit(void)
{
    printk(KERN_ERR DEV_NAME ": tx433_exit\n");
    iounmap(gpio);
    misc_deregister(&tx433_misc_device);
    gpio1 = 0;
    gpio = NULL;
}

subsys_initcall(tx433_init);
module_exit(tx433_exit);

MODULE_AUTHOR("Jack Whitham");
MODULE_DESCRIPTION("433MHz transmitter driver");
MODULE_LICENSE("GPL");

