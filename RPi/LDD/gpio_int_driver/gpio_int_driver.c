// Link : http://lightsurge2.blogspot.com/2014/05/using-rapsberry-pis-gpio-as-interrupt.html

#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/interrupt.h>
#include <linux/gpio.h>

#include <linux/init.h>
#include <linux/delay.h>

// Define the GPIO number which is used as source of interrupt
#define GPIO_INT_NUM    23

// The following info will be displayed in /proc/interrupt
#define GPIO_INT_DESC    "GPIO23 Interrupt switch"

// TODO
#define GPIO_INT_DEVICE_DESC    "GPIO23"

/****************************************************************************/
/* Interrupts variables block                                               */
/****************************************************************************/
short int irq_any_gpio = 0;
 
/****************************************************************************/
/* Tasklets - provides the actual processing of the IRQ using the           */
/* bottom half approach (lower priority handlers). These are non re-entrant */
/* handlers (different from sofirqs). This is where we are going to set the */
/* processing of our leds (actually just output something to leds).         */
/****************************************************************************/
static void led_output_handler(unsigned long data);

DECLARE_TASKLET(ledtasklet, led_output_handler, 0L)

static void led_output_handler(unsigned long data)
{
    tasklet_disable(&ledtasklet);

    printk("%s: Takslets executed!\n", __FUNCTION__);

    tasklet_enable(&ledtasklet);
}

/****************************************************************************/
/* IRQ handler                                                              */
/****************************************************************************/
static irqreturn_t gpio_irq_handler(int irq, void *dev_id, struct pt_regs *regs) 
{
    unsigned log flags;

    // disable hard interrupts (remember them in flag 'flags')
    local_irq_save(flags);

    // We defer handling of our IRQ to tasklets
    tasklet_schedule(&ledtasklet);

    // restore hard interrupts
    local_irq_restore(flags);

    return IRQ_HANDLED;
}

/****************************************************************************/
/* GPIO initialization function for configuring GPIO_NUM as interrupt source*/
/****************************************************************************/
void gpio_int_config(void) 
{
    // TODO : Explain
    if (gpio_request(GPIO_INT_NUM, GPIO_INT_DESC)) {
        printk("%s: GPIO request failure: %s\n", __FUNCTION__, 
                                          GPIO_INT_DESC);
        return;
    }

    // Instruct the kernel that the GPIO will be used as input
    if (gpio_direction_input(GPIO_INT_NUM) < 0)
    {
        printk("%s: GPIO direction setting failure:%s\n", __FUNCTION__,
                                                   GPIO_INT_DESC);
        return;
    }

    // Map the GPIO to IRQ
    if ((irq_any_gpio = gpio_to_irq()) <0) {
        printk("%s: GPIO to IRQ mapping failure: %s\n", __FUNCTION__, 
                                                 GPIO_INT_DESC);
        return;
    }

    printk("%s: GPIO mapped to interrupt %d\n", __FUNCTION__, 
                                         irq_any_gpio);

    if (request_irq(irq_any_gpio,
                   (irq_handler_t ) gpio23_irq_handler,
                   IRQF_TRIGGER_RISING,
                   GPIO_INT_DESC,
                   GPIO_INT_DEVICE_DESC)) 
    {
		printk("%s: IRQ request failure\n", __FUNCTION__);
		return;
    }

    return;
}

/****************************************************************************/
/* Release interrupt(s).                                                    */
/****************************************************************************/
void gpio_int_release(void) 
{  
    free_irq(irq_any_gpio, GPIO_INT_DEVICE_DESC);
    gpio_free(GPIO_INT_NUM);

   return;
}

/****************************************************************************/
/* Module init / cleanup block.                                             */
/****************************************************************************/
int gpio_int_init(void) 
{  
   printk(KERN_NOTICE "Hello !\n");
   gpio_int_config();
  
   return 0;
}
  
void gpio_int_cleanup(void) 
{
   printk(KERN_NOTICE "Goodbye\n");
   gpio_int_release();
  
   return;
}


module_init(gpio_int_init);
module_exit(gpio_int_cleanup);

/****************************************************************************/
/* Module licensing/description block.                                      */
/****************************************************************************/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vergil Cola <vpcola@gmail.com>");
MODULE_AUTHOR("Pascal Le Boudin <fxrdude@gmail.com>");
MODULE_DESCRIPTION("Sample GPIO Interrupt Handler");

