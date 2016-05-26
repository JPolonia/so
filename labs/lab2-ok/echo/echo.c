
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>


#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/cdev.h>

#include <asm/system.h>
#include <asm/uaccess.h>


#include "echo.h"

MODULE_LICENSE("Dual BSD/GPL");

struct echo_dev *echo_device;
int echo_major =   ECHO_MAJOR;
int echo_minor = 0;

module_param(echo_major, int, 0000);
module_param(echo_minor, int, 0000);

struct file_operations echo_fops ={
    
    .owner =    THIS_MODULE,
    .llseek = 	no_llseek,
    .open  =    echo_open,
    .release =  echo_release,
    .write =	echo_write,
    .read =		echo_read,

    //.ioctl = 	echo_ioctl,
    
};


int echo_open(struct inode *inode, struct file *filp){
    
    struct echo_dev *dev;
    
    nonseekable_open(inode, filp);

    dev = container_of(inode->i_cdev, struct echo_dev, cdev);
    filp->private_data = dev;
    
    printk(KERN_NOTICE "Private_data successeful allocated!\n");
    
    return 0;
}

int echo_release(struct inode *inode, struct file *filp){
	
	printk(KERN_NOTICE "Private_data successeful released!\n");
    
    return 0;
}


static void echo_create_cdev(struct echo_dev *dev){
    
    int dev_numb = MKDEV(echo_major, echo_minor);
    int result;
    
    cdev_init(&dev->cdev, &echo_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &echo_fops;
    
    result = cdev_add(&dev->cdev, dev_numb, 1);
    
    if(result<0)
        printk(KERN_WARNING "Error adding device to cdev! - %d\n", result);
        
}


int echo_init(void)
{

    dev_t dev = 0;
    int result;
    
    
    if(echo_major){
        dev = MKDEV(echo_major, echo_minor);
        result = register_chrdev_region(dev, 1, "echo");
    }
    else{
        result = alloc_chrdev_region(&dev, 0, 1, "echo");
        echo_major = MAJOR(dev);
    }
    if(result < 0){ //Error
        printk(KERN_WARNING "Echo can't get MAJOR: %d\n", echo_major);
        return result;
    }
        
    echo_device = kmalloc(sizeof(struct echo_dev),GFP_KERNEL);
    memset(echo_device, 0, sizeof(struct echo_dev));
    
    echo_create_cdev(echo_device);
        
        
    printk(KERN_ALERT "Major - %d / Minor - %d\n", echo_major, echo_minor);

    return 0;
}

void echo_exit(void)
{
    
    cdev_del(&echo_device->cdev);
    kfree(echo_device);
        
    unregister_chrdev_region(MKDEV (echo_major, echo_minor), 1);

    printk(KERN_ALERT "Major and minor device freed!\n");
}

module_init(echo_init);
module_exit(echo_exit);
