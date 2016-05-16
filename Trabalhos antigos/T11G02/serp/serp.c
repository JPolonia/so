#include "serial_reg.h"
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/delay.h>

MODULE_LICENSE("Dual BSD/GPL");

dev_t device;
static int Major;
static struct cdev *cdp;


static int port = 0x3f8;  //io port number(default)


int dev_open (struct inode *inode, struct file *filp){
	extern int nonseekable_open(struct inode * inode, struct file * filp);
  
	filp->private_data = cdp;
	printk(KERN_ALERT "Open\n");
	return 0;
}

int dev_release (struct inode *inode, struct file *filp){
	printk(KERN_ALERT "Release\n");
	return 0;
}

ssize_t dev_write (struct file *filp, const char __user *buf, size_t count,loff_t *f_pos){
  
  
      int i, l=0;
      unsigned long cpy;
      unsigned char lsr=0;
      char dest[count];
      printk(KERN_ALERT "Writing\n");


     cpy = copy_from_user(dest,buf,count);
     if(cpy!=0)
	return -EFAULT;
     
     for(i=0;i<count;i++) {
     lsr = inb(port+UART_LSR);
     
        while(l!=1){
	  set_current_state(TASK_INTERRUPTIBLE);
	  schedule_timeout(0.2*HZ);
      if(lsr & UART_LSR_THRE){
	outb(dest[i] ,port+UART_TX);
	l=1;
	}
	}
	l=0;
	if(i>100){
	  printk(KERN_ALERT "End cicle\n");
	  return 0;
	}
     }         
      printk(KERN_ALERT "End Writing\n");
 
      return 0;   
}

ssize_t dev_read(struct file *filep, char __user *buff, size_t count, loff_t *offp){
  
      unsigned char lsr,info;
      int l=0;
      printk(KERN_ALERT "Reading\n");
      
      while (info!='\n'){
	while(l!=1){
	  set_current_state(TASK_INTERRUPTIBLE);
	  schedule_timeout(0.5*HZ);
	  lsr = inb(port+UART_LSR);
      	  if(lsr & UART_LSR_DR){
	    info=inb(port+UART_RX);
	    printk("%c",info);
	    l=1;
	  }
	}
       l=0;
      }
      printk(KERN_ALERT "\n");
      return 1;
      
      
}

struct file_operations fops = {
	.read = dev_read,
	.write = dev_write,
	.open = dev_open,
	.release = dev_release,
	//.llseek = echo_llseek,
};


static int init_uart (void){ // UART Initializer function
  
	int k;
  	unsigned char lcr=0;
	
	 if (! request_region(port, 8, "serp")) {
	    printk(KERN_INFO "short: can't get I/O\n");
	    return -ENODEV;
              }

	lcr = lcr | UART_LCR_WLEN8; 
	lcr = lcr | UART_LCR_STOP;
	lcr = lcr | UART_LCR_EPAR;
	outb(lcr, port+UART_LCR);//8-bit chars, 2 stop bits, parity even
	
	
	lcr = lcr | UART_LCR_DLAB;
	outb(lcr, port+UART_LCR);
	outb(UART_DIV_1200 & 0x00FF, port+UART_DLL);
	outb((UART_DIV_1200 & 0xFF00)<<8, port+UART_DLM);
	lcr &= ~UART_LCR_DLAB;
	outb(lcr, port+UART_LCR);
	
	outb(0x0,port+UART_IER);
	
	
	printk(KERN_ALERT "Init\n");
	
	k=alloc_chrdev_region(&device, 0, 1, "serp");
	if (k < 0) {
	  printk(KERN_INFO "Major number allocation is failed\n");
	return k; 
	}     
	
	Major=MAJOR(device);
	printk(KERN_ALERT "Major n:%d\n",Major);
	
	cdp=cdev_alloc();
	cdp->ops = &fops; 
	cdp->owner = THIS_MODULE; 
	
	k = cdev_add( cdp,device,1);
	if(k < 0 ){
	printk(KERN_INFO "Unable to add cdev");
	return k;
	}

	
	return 0;
}

void exit_uart (void){
   release_region(port,8);
   cdev_del(cdp);
   unregister_chrdev_region(device, 1);
   printk(KERN_ALERT "Exit\n");
}

module_init(init_uart);
module_exit(exit_uart);
