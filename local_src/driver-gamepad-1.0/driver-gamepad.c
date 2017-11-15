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

// Declare functions
static int my_open (struct inode *inode, struct file *filp);
static int my_release(struct inode *inode, struct file *filp);
static ssize_t my_read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
static int my_fasync(int fd, struct file *filp, int mode);
static int __init template_init(void);
static int my_probe(struct platform_device *dev);
static irqreturn_t interrupt_handler(int irq, void *dev_id, struct pt_regs *regs);
static void __exit template_cleanup(void);
static int my_remove(struct platform_device *dev);

// Declare variables
static struct file_operations my_fops = { 
	.owner = THIS_MODULE,
	.read = my_read,
	.open = my_open,
	.release = my_release, 
	.fasync = my_fasync
};
struct cdev my_cdev;
struct class *cl;
struct resource *res;
struct resource *gpioRes;
struct fasync_struct *async_queue;

dev_t device;
int err;
int irqEven;
int irqOdd;
unsigned int buttonValues;

// ------------------------ Platform device set up ----------------------
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

// ---------------------------- Interrupt related -----------------------------
static int my_fasync(int fd, struct file *filp, int mode)
{
	return fasync_helper(fd, filp, mode, &async_queue);
}

static irqreturn_t interrupt_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	buttonValues = ioread32(GPIO_PC_DIN);
	buttonValues = ~buttonValues;
	buttonValues &= 255;
	iowrite32(ioread32(GPIO_IF),GPIO_IFC);
	if (async_queue) 
	{
		kill_fasync(&async_queue, SIGIO, POLL_IN);
	}
	return IRQ_HANDLED;
}

// ---------------------- Visible functions for game --------------------------
static int my_open (struct inode *inode, struct file *filp)
{
	printk("Open device \n");
	return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
	printk("Release device \n");
	return 0;
}

static ssize_t my_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	
	return 4-copy_to_user(buff, &buttonValues, 4);
}

// -------------------------- Intialize ---------------------------------------
static int __init template_init(void)
{		
	platform_driver_register(&my_driver);
	return 0;
}


static int my_probe(struct platform_device *dev)
{
	// Device number
	alloc_chrdev_region(&device, 0, 1, DEVICE_NAME);

	// Set up char driver
	cdev_init(&my_cdev, &my_fops);
	cdev_add(&my_cdev, device, 1);
	
	// Make visible in filesystem
	cl = class_create(THIS_MODULE, DEVICE_NAME);
	device_create(cl, NULL, device, NULL, DEVICE_NAME);

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);

	// Set up interrupt
	irqEven = platform_get_irq(dev, 0);
	irqOdd = platform_get_irq(dev, 1);

	gpioRes = request_mem_region(res->start, res->end - res->start, DEVICE_NAME);

	iowrite32(0x33333333,GPIO_PC_MODEL);
	iowrite32(0xFF,GPIO_PC_DOUT);

	request_irq(irqEven, (irq_handler_t)interrupt_handler, 0, DEVICE_NAME, &my_cdev);
	request_irq(irqOdd, (irq_handler_t)interrupt_handler, 0, DEVICE_NAME, &my_cdev);

	iowrite32(0x22222222,GPIO_EXTIPSELL);
	iowrite32(0xFF,GPIO_EXTIRISE);
	iowrite32(0xFF,GPIO_EXTIFALL);
	iowrite32(0xFF, GPIO_IEN);

	printk("Driver set up complete \n");

	return 0;
}

// ------------------------- Terminate ----------------------------------------
// Called when template_cleanup is called
static int my_remove(struct platform_device *dev)
{
	iowrite32(0, GPIO_IEN);
 	release_mem_region(res->start, res->end - res->start);

	free_irq(irqEven, NULL);
	free_irq(irqOdd, NULL);
	
	device_destroy(cl, device);
	class_destroy(cl);
	cdev_del(&my_cdev);
	unregister_chrdev_region(device, 1);
	printk("Short life for a small module...\n");
	return 0;
}

// Remove driver
static void __exit template_cleanup(void)
{
	platform_driver_unregister(&my_driver);
}

module_init(template_init);
module_exit(template_cleanup);

MODULE_DESCRIPTION("Small module, very useful. \n");
MODULE_LICENSE("GPL");
