/*
 * This is a demo Linux kernel module.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include "efm32gg.h"
#include <linux/ioport.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/signal.h>
#include <linux/platform_device.h>
#include <linux/fs.h>


#define DEVICE_NAME "gamepad"

static int my_open (struct inode *inode, struct file *filp);
static int my_release(struct inode *inode, struct file *filp);
static ssize_t my_read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
static ssize_t my_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);

struct fasync_struct *async_queue;

static int my_fasync(int fd, struct file *filp, int mode){
	return fasync_helper(fd, filp, mode, &async_queue);
}

static struct file_operations my_fops = { 
	.owner = THIS_MODULE,
	.read = my_read,
	.write = my_write,
	.open = my_open,
	.release = my_release, 
	.fasync = my_fasync
};

/*
 * template_init - function to insert this module into kernel space
 *
 * This is the first of two exported functions to handle inserting this
 * code into a running kernel
 *
 * Returns 0 if successfull, otherwise -1
 */

dev_t device;

struct cdev my_cdev;

struct class *cl;

int err;

struct resource *res;

struct resource *gpioRes;

int irqEven;
int irqOdd;

unsigned int buttonValues;

static irqreturn_t interrupt_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	printk("Dette er et interrupt");
	printk("read buttons");
	buttonValues = ioread32(GPIO_PC_DIN);
	buttonValues = ~buttonValues;
	buttonValues &= 255;
	printk("buttonValues: %d", buttonValues);
	iowrite32(ioread32(GPIO_IF),GPIO_IFC);

	if (async_queue) {
		kill_fasync(&async_queue, SIGIO, POLL_IN);
	}
	return IRQ_HANDLED;
}


static int my_probe(struct platform_device *dev)
{
	printk("yo");
	err = alloc_chrdev_region(&device, 0, 1, DEVICE_NAME);
	printk("Major %d\n", err);
	cdev_init(&my_cdev, &my_fops);
	printk("Major %d\n", err);
	err = cdev_add(&my_cdev, device, 1);
	printk("Major %d\n", err);
	cl = class_create(THIS_MODULE, DEVICE_NAME);
	printk("cl: %p\n", cl);
	err = device_create(cl, NULL, device, NULL, DEVICE_NAME);
	printk("Major %d\n", err);
	printk("Major %d\n", MAJOR(device));
	printk("Hello World, here is your module speaking\n");
	res = platform_get_resource(dev, IORESOURCE_MEM, 0);

	printk("Hentet res%p\n", res);
	irqEven = platform_get_irq(dev, 0);
	irqOdd = platform_get_irq(dev, 1);
	gpioRes = request_mem_region(res->start, res->end - res->start, DEVICE_NAME);
	printk("requestet mem%p\n", gpioRes);

	iowrite32(0x33333333,GPIO_PC_MODEL);
	iowrite32(0xFF,GPIO_PC_DOUT);

	request_irq(irqEven,interrupt_handler,NULL,DEVICE_NAME, NULL);
	request_irq(irqOdd,interrupt_handler,NULL,DEVICE_NAME, NULL);

	iowrite32(0x22222222,GPIO_EXTIPSELL);
	iowrite32(0xFF,GPIO_EXTIRISE);
	iowrite32(0xFF,GPIO_EXTIFALL);
	iowrite32(0xFF, GPIO_IEN);
	return 0;
}

static int my_remove(struct platform_device *dev)
{
	printk("Short life for a small module...\n");
}

static const struct of_device_id my_of_match[] = {
	{ .compatible = "tdt4258", },
	{},
};

MODULE_DEVICE_TABLE(of, my_of_match);

static struct platform_driver my_driver = {
	.probe = my_probe,
	.remove = my_remove,
	.driver = {
		.name = "my",
		.owner = THIS_MODULE,
		.of_match_table = my_of_match,
	},
};

static int __init template_init(void)
{		
	platform_driver_register(&my_driver);
	 return 0;
}

/*
 * template_cleanup - function to cleanup this module from kernel space
 *
 * This is the second of two exported functions to handle cleanup this
 * code from a running kernel
 */

static int my_open (struct inode *inode, struct file *filp)
{
	printk("open device");
	return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
	printk("release device");
	return 0;
}

static ssize_t my_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	
	return 4-copy_to_user(buff, &buttonValues, 4);
}


static ssize_t my_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
	printk("write buttons, say what?");
	return 1;
}




static void __exit template_cleanup(void)
{
	 printk("Short life for a small module...\n");
}

module_init(template_init);
module_exit(template_cleanup);

MODULE_DESCRIPTION("Small module, demo only, not very useful.");
MODULE_LICENSE("GPL");

