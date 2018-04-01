/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Derived from many drivers using generic_serial interface.
 *
 * Copyright (C) 2015 Brevera Technologies Pvt Ltd <venkatesh@brevera.in>
 *
 * Serial driver for BCM2835 mini UART.
 *
 * Hardware flow control, Console support not implimented. Works only with
 * 115200 8N1 configuration. 
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/clk.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/sysrq.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/of.h>

#define BCM285_NR_UARTS	1

#define GPFSEL1 0x20200004
#define GPSET0  0x2020001C
#define GPCLR0  0x20200028
#define GPPUD       0x20200094
#define GPPUDCLK0   0x20200098

#define AUX_ENABLES     0x20215004
#define AUX_MU_IO_REG   0x20215040


#define AUX_MU_IO_REG_OFFSET   0
#define AUX_MU_IER_REG_OFFSET  0x4
#define AUX_MU_IIR_REG_OFFSET  0x8
#define AUX_MU_LCR_REG_OFFSET  0xC
#define AUX_MU_MCR_REG_OFFSET  0x10
#define AUX_MU_LSR_REG_OFFSET  0x14
#define AUX_MU_MSR_REG_OFFSET  0x18
#define AUX_MU_SCRATCH_OFFSET  0x1C
#define AUX_MU_CNTL_REG_OFFSET 0x20
#define AUX_MU_STAT_REG_OFFSET 0x24
#define AUX_MU_BAUD_REG_OFFSET 0x28

#define IRQ_BASIC 0x2000B200
#define IRQ_PEND1 0x2000B204
#define IRQ_PEND2 0x2000B208
#define IRQ_FIQ_CONTROL 0x2000B210
#define IRQ_ENABLE1 0x2000B210
#define IRQ_ENABLE2 0x2000B214
#define IRQ_ENABLE_BASIC 0x2000B218
#define IRQ_DISABLE1 0x2000B21C
#define IRQ_DISABLE2 0x2000B220
#define IRQ_DISABLE_BASIC 0x2000B224

#define BCM_MINI_UART_REG_SIZE 48
#define PORT_BCM285 110
static struct uart_port ports[BCM285_NR_UARTS];

/*
 * handy uart register accessor
 */
static inline unsigned int bcm_uart_readl(struct uart_port *port,
					 unsigned int offset)
{
	return __raw_readl(port->membase + offset);
}

static inline void bcm_uart_writel(struct uart_port *port,
				  unsigned int value, unsigned int offset)
{
	__raw_writel(value, port->membase + offset);
}

/*
 * serial core request to check if uart tx fifo is empty
 */
static unsigned int bcm_uart_tx_empty(struct uart_port *port)
{
	unsigned int val;
	val = bcm_uart_readl(port, AUX_MU_LSR_REG_OFFSET);
	return (val & 0x20) ? 1 : 0;
}

/*
 * serial core request to set RTS and DTR pin state and loopback mode
 */
static void bcm_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	// not implimented. 
}

/*
 * serial core request to return RI, CTS, DCD and DSR pin state
 */
static unsigned int bcm_uart_get_mctrl(struct uart_port *port)
{
	// not implimented. 
	return 0;
}


/*
 * serial core request to disable tx ASAP (used for flow control)
 */
static void bcm_uart_stop_tx(struct uart_port *port)
{
	// not implimented. 
}

/*
 * serial core request to (re)enable tx
 */
static void bcm_uart_start_tx(struct uart_port *port)
{
	bcm_uart_writel(port, 7, AUX_MU_IER_REG_OFFSET);
	bcm_uart_writel(port, 3, AUX_MU_LCR_REG_OFFSET);
}

/*
 * serial core request to stop rx, called before port shutdown
 */
static void bcm_uart_stop_rx(struct uart_port *port)
{
	// not implimented. 
}

/*
 * serial core request to enable modem status interrupt reporting
 */
static void bcm_uart_enable_ms(struct uart_port *port)
{
	// not implimented. 
}

/*
 * serial core request to start/stop emitting break char
 */
static void bcm_uart_break_ctl(struct uart_port *port, int ctl)
{
	// not implimented. 
}

/*
 * return port type in string format
 */
static const char *bcm_uart_type(struct uart_port *port)
{
	return (port->type == PORT_BCM285) ? "BCM285_uart" : NULL;
}

/*
 * fill tx fifo with chars to send, stop when fifo is about to be full
 * or when all chars have been sent.
 */
static void bcm_uart_do_tx(struct uart_port *port)
{
	struct circ_buf *xmit;
	unsigned int val, max_count;

	if (port->x_char) {
		bcm_uart_writel(port, port->x_char, AUX_MU_IO_REG_OFFSET);
		port->icount.tx++;
		port->x_char = 0;
		return;
	}

	if (uart_tx_stopped(port)) {
		bcm_uart_stop_tx(port);
		return;
	}

	xmit = &port->state->xmit;
	if (uart_circ_empty(xmit))
		goto txq_empty;

	val = bcm_uart_readl(port, AUX_MU_STAT_REG_OFFSET);
	val = (val & (0xF<<23)) >> 23;
	max_count = port->fifosize - val;

	while (max_count--) {
		unsigned int c;

		c = xmit->buf[xmit->tail];
		bcm_uart_writel(port, c, AUX_MU_IO_REG_OFFSET);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		goto txq_empty;
	return;

txq_empty:
	/* nothing to send, disable transmit interrupt */
	bcm_uart_writel(port, 5, AUX_MU_IER_REG_OFFSET);
	return;
}


/*
 * read all chars in rx fifo and send them to core
 */
static void bcm_uart_do_rx(struct uart_port *port)
{
	struct tty_port *tty_port = &port->state->port;
	unsigned int max_count;
	
	
	/* limit number of char read in interrupt, should not be
	 * higher than fifo size anyway since we're much faster than
	 * serial port */
	
	max_count = 8;
	do {
		unsigned int iestat, c, cstat;
		char flag;

		iestat = bcm_uart_readl(port, AUX_MU_STAT_REG_OFFSET);
		if (!(iestat & 0xF0000))
			break;

		cstat = c = bcm_uart_readl(port, AUX_MU_IO_REG_OFFSET);
		port->icount.rx++;
		flag = TTY_NORMAL;
		c &= 0xff;
		tty_insert_flip_char(tty_port, c, flag);

	} while (--max_count);
		
	spin_unlock(&port->lock);
	tty_flip_buffer_push(tty_port);
	spin_lock(&port->lock);
}



/*
 * process uart interrupt
 */
static irqreturn_t bcm_uart_interrupt(int irq, void *dev_id)
{
	struct uart_port *port;
	unsigned int irqstat;
	
	port = dev_id;
	spin_lock(&port->lock);

	irqstat = bcm_uart_readl(port, AUX_MU_IIR_REG_OFFSET);
	
	if(irqstat & 0x4)
		bcm_uart_do_rx(port);
	if(irqstat & 0x2)
		bcm_uart_do_tx(port);
	
	spin_unlock(&port->lock);
	return IRQ_HANDLED;
}

/*
 * enable rx & tx operation on uart
 */
static void bcm_uart_enable(struct uart_port *port)
{
	bcm_uart_writel(port, 0x3, AUX_MU_CNTL_REG_OFFSET);
}

/*
 * disable rx & tx operation on uart
 */
static void bcm_uart_disable(struct uart_port *port)
{
	bcm_uart_writel(port, 0, AUX_MU_CNTL_REG_OFFSET);
}

/*
 * clear all unread data in rx fifo and unsent data in tx fifo
 */
static void bcm_uart_flush(struct uart_port *port)
{
	bcm_uart_writel(port, 0xC6, AUX_MU_IIR_REG_OFFSET);
}

/*
 * serial core request to initialize uart and start rx operation
 */
static int bcm_uart_startup(struct uart_port *port)
{
	int ret;
	/* mask all irq and flush port */
	bcm_uart_disable(port);
	bcm_uart_writel(port, 0, AUX_MU_IER_REG_OFFSET);
	bcm_uart_flush(port);

	ret = request_irq(port->irq, bcm_uart_interrupt, 0,
			  bcm_uart_type(port), port);
	if (ret)
		return ret;
	bcm_uart_writel(port, 0x5, AUX_MU_IER_REG_OFFSET);
	bcm_uart_enable(port);
	return 0;
}

/*
 * serial core request to flush & disable uart
 */
static void bcm_uart_shutdown(struct uart_port *port)
{
	bcm_uart_disable(port);
	bcm_uart_flush(port);
	free_irq(port->irq, port);
}

/*
 * serial core request to change current uart setting
 * Currently only 115200 8N1 configuration is supported. 
 */
static void bcm_uart_set_termios(struct uart_port *port,
				 struct ktermios *new,
				 struct ktermios *old)
{
	unsigned long flags;
	spin_lock_irqsave(&port->lock, flags);

	/* disable uart while changing speed */
	bcm_uart_disable(port);
	bcm_uart_flush(port);

	bcm_uart_writel(port, 0x3, AUX_MU_LCR_REG_OFFSET);
	bcm_uart_writel(port, 270, AUX_MU_BAUD_REG_OFFSET);
	
	bcm_uart_enable(port);
	spin_unlock_irqrestore(&port->lock, flags);
}

/*
 * serial core request to claim uart iomem
 */
static int bcm_uart_request_port(struct uart_port *port)
{
	unsigned int size;
	
	size = BCM_MINI_UART_REG_SIZE;
	if (!request_mem_region(port->mapbase, size, "BCM285")) {
		dev_err(port->dev, "Memory region busy\n");
		return -EBUSY;
	}

	port->membase = ioremap(port->mapbase, size);
	if (!port->membase) {
		dev_err(port->dev, "Unable to map registers\n");
		release_mem_region(port->mapbase, size);
		return -EBUSY;
	}
	return 0;
}

/*
 * serial core request to release uart iomem
 */
static void bcm_uart_release_port(struct uart_port *port)
{
	release_mem_region(port->mapbase, BCM_MINI_UART_REG_SIZE);
	iounmap(port->membase);
}

/*
 * serial core request to do any port required autoconfiguration
 */
static void bcm_uart_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		if (bcm_uart_request_port(port))
			return;
		port->type = PORT_BCM285;
	}
}

/*
 * serial core request to check that port information in serinfo are
 * suitable
 */
static int bcm_uart_verify_port(struct uart_port *port,
				struct serial_struct *serinfo)
{
	if (port->type != PORT_BCM285)
		return -EINVAL;
	if (port->irq != serinfo->irq)
		return -EINVAL;
	if (port->iotype != serinfo->io_type)
		return -EINVAL;
	if (port->mapbase != (unsigned long)serinfo->iomem_base)
		return -EINVAL;
	return 0;
}

/* serial core callbacks */
static struct uart_ops bcm_uart_ops = {
	.tx_empty	= bcm_uart_tx_empty,
	.get_mctrl	= bcm_uart_get_mctrl,
	.set_mctrl	= bcm_uart_set_mctrl,
	.start_tx	= bcm_uart_start_tx,
	.stop_tx	= bcm_uart_stop_tx,
	.stop_rx	= bcm_uart_stop_rx,
	.enable_ms	= bcm_uart_enable_ms,
	.break_ctl	= bcm_uart_break_ctl,
	.startup	= bcm_uart_startup,
	.shutdown	= bcm_uart_shutdown,
	.set_termios	= bcm_uart_set_termios,
	.type		= bcm_uart_type,
	.release_port	= bcm_uart_release_port,
	.request_port	= bcm_uart_request_port,
	.config_port	= bcm_uart_config_port,
	.verify_port	= bcm_uart_verify_port,
};


static struct uart_driver bcm_uart_driver = {
	.owner		= THIS_MODULE,
	.driver_name	= "ttyS",
	.dev_name	= "ttyS",
	.major		= 205,
	.minor		= 65,
	.nr		= BCM285_NR_UARTS,
};

/*
 * platform driver probe/remove callback
 */
static int bcm_uart_probe(struct platform_device *pdev)
{
	struct uart_port *port;
	int ret;
	unsigned int count;
	count = 0;

	__raw_writel(1,(uint32_t *)__io_address(AUX_ENABLES));

	if (pdev->dev.of_node)
		pdev->id = of_alias_get_id(pdev->dev.of_node, "uart");
	if (pdev->id < 0 || pdev->id >= BCM285_NR_UARTS)
		return -EINVAL;
	if (ports[pdev->id].membase)
		return -EBUSY;
		
	port = &ports[pdev->id];
	memset(port, 0, sizeof(*port));
	port->iotype = UPIO_MEM;
	port->mapbase = AUX_MU_IO_REG;
	
	port->irq = 29;
	port->ops = &bcm_uart_ops;
	port->flags = UPF_BOOT_AUTOCONF;
	port->dev = &pdev->dev;
	port->fifosize = 8;
	port->uartclk = 250000000;
	port->line = pdev->id;

	ret = uart_add_one_port(&bcm_uart_driver, port);
	if (ret) {
		ports[pdev->id].membase = 0;
		return ret;
	}
	platform_set_drvdata(pdev, port);
	return 0;
}

static int bcm_uart_remove(struct platform_device *pdev)
{
	struct uart_port *port;

	port = platform_get_drvdata(pdev);
	uart_remove_one_port(&bcm_uart_driver, port);
	/* mark port as free */
	ports[pdev->id].membase = 0;
	return 0;
}

/*
 * platform driver stuff
 */
static struct platform_driver bcm_uart_platform_driver = {
	.probe	= bcm_uart_probe,
	.remove	= bcm_uart_remove,
	.driver	= {
		.owner = THIS_MODULE,
		.name  = "ttyS",
	},
};


static void drivertest_device_release(struct device *dev)
{
}

static struct platform_device drivertest_device = {
	.name = "ttyS",
	.id = 0,
	.dev = {
		.release = drivertest_device_release,
	},
};

static int __init bcm_uart_init(void)
{
	int ret;
	ret = uart_register_driver(&bcm_uart_driver);
	if (ret){
		return ret;
	}
	platform_device_register(&drivertest_device);
	ret = platform_driver_register(&bcm_uart_platform_driver);
	if (ret)
	{
		uart_unregister_driver(&bcm_uart_driver);
	}

	return ret;
}

static void __exit bcm_uart_exit(void)
{
	platform_driver_unregister(&bcm_uart_platform_driver);
	platform_device_unregister(&drivertest_device);
	uart_unregister_driver(&bcm_uart_driver);
}

module_init(bcm_uart_init);
module_exit(bcm_uart_exit);

MODULE_AUTHOR("Brevera <venkatesh@brevera.in>");
MODULE_DESCRIPTION("BCM285 mini uart driver");
MODULE_LICENSE("GPL");
