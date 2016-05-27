/*                                                     
 * $Id: echo.c,v 1.5 2004/10/26 03:32:21 corbet Exp $ 
 */                                                    
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include "serp.h"
#include "serial_reg.h"



/*
void *kmalloc( size_t size, GFP_KERNEL); 
void *kzalloc( size_t size, GFP_KERNEL);
void kfree(void *obj)
int alloc_chrdev_region(dev_t *dev, unsigned int firstminor, unsigned int count, char *name);
void unregister_chrdev_region(dev_t first, unsigned int count);


*/

MODULE_LICENSE("Dual BSD/GPL");


int echo_open(struct inode *inodep, struct file *filep);
int echo_release(struct inode *inodep, struct file *filep);
ssize_t echo_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp);
ssize_t echo_read(struct file *filep, char __user *buff, size_t count, loff_t *offp);

struct echo_dev{
	struct cdev cdev;
	unsigned short int Adr_base;
	int cnt;
};

dev_t device;
struct echo_dev *cdp;






struct file_operations fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.open = echo_open,
	.release = echo_release,
	.write = echo_write,
	.read = echo_read,
};

int echo_open(struct inode *inodep, struct file *filep) {
	struct echo_dev *edevp;
	nonseekable_open(inodep,filep);
	edevp = container_of(inodep->i_cdev, struct echo_dev, cdev);
	filep->private_data = edevp;
	edevp->cnt=0;
	printk(KERN_ALERT "\n echo_open call\n");
	return 0;	
}

int echo_release(struct inode *inodep, struct file *filep) {
	printk(KERN_ALERT "\n echo_realease call\n");
	return 0;
}

ssize_t echo_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp){
	int i;
	char *buffer;
	unsigned short int Adr_lsr, Adr_tx;
	struct echo_dev *edevp;
	buffer = kmalloc(sizeof(char)*(count+1),GFP_KERNEL);
	if(buffer==NULL){
		printk(KERN_ALERT "Can't allocat memory for buffer\n");
		return -ENOMEM;
	}
	
	if(copy_from_user(buffer,buff,count)!=0){
		kfree(buffer);
		return -EFAULT;
	}
	edevp = (struct echo_dev *) filep->private_data;
	Adr_lsr = edevp->Adr_base + UART_LSR;
	Adr_tx = edevp->Adr_base + UART_TX;
	buffer[count]='\0';
	//printk(KERN_NOTICE "Function Write:\n String : %s", buffer);
	for(i=0;i<count;i++){
		while((inb(Adr_lsr)& UART_LSR_THRE) == 0){
			msleep_interruptible(1);
		}
		outb(buffer[i],Adr_tx);
	}	
	edevp->cnt+=count;
	kfree(buffer);
	return count;
}

ssize_t echo_read(struct file *filep, char __user *buff, size_t count, loff_t *offp){
	char* buffer;
	unsigned char value;
	unsigned short int Adr_lsr, Adr_rx;
	int i;
	struct echo_dev *edevp;
	if (count==0) return 0;
	edevp = (struct echo_dev *) filep->private_data;
	buffer = kmalloc(count*sizeof(char), GFP_KERNEL);
	if(buffer == NULL){
		return -1;
	}
	Adr_lsr = edevp->Adr_base + UART_LSR;
	Adr_rx = edevp->Adr_base + UART_RX;
	for(i=0;i<count;i++){
		value = inb(Adr_lsr);
		while((value & UART_LSR_DR) == 0 ){
			msleep_interruptible(1);
			value = inb(Adr_lsr);
		}
		if( value & UART_LSR_OE){
			kfree(buffer);
			return -EIO;	
		}else{
			buffer[i] = inb(Adr_rx);
		}
	}
	i = copy_to_user((void __user*) buff, (const void*) buffer, i);
	kfree(buffer);
	return count-i;
}








static int echo_init(void)
{
	int i;
	struct resource *port;
	// allocate Major Device
	if(alloc_chrdev_region(&device,0,ECHO_DEVS,"serp") < 0){
		printk(KERN_ALERT "Error!MAJOR DEVICE NUMBER can't be allocat");
		return -1;
	}
	// allocates space for devices
	cdp = kmalloc(ECHO_DEVS*sizeof(struct echo_dev),GFP_KERNEL);
	if(cdp == NULL){
		printk(KERN_ALERT "Error!MIinor DEVICEs can't be allocat");
		goto error;
	}
	//inicializate devices
	for(i=0;i<ECHO_DEVS;i++){
		cdev_init(&cdp[i].cdev,&fops);
		cdp[i].cdev.owner = THIS_MODULE;
		cdp[i].Adr_base = ADR_COM1;
		if((cdev_add(&cdp[i].cdev,device,1))<0){
			printk(KERN_ALERT "Error! cannot register device");
			goto error;			
		}
	}
	
	port = request_region(ADR_COM1,1,"serp");
	if(port == NULL){
		printk(KERN_ALERT "Error! cannot acess i/o ports");
		goto error;
	}	
	
	//inicializate endereços
	outb(0x00, ADR_COM1+UART_IER);
	outb(UART_LCR_DLAB,ADR_COM1+UART_LCR);
	outb(UART_DIV_1200, ADR_COM1+UART_DLL);
  	outb(0x00, ADR_COM1+UART_DLM);
	outb(( UART_LCR_STOP |UART_LCR_EPAR | UART_LCR_WLEN8), ADR_COM1+UART_LCR);
	
	// mensagem sucesso
	printk(KERN_ALERT "MAJOR DEVICE NUMBER allocat with sucess. It's value is %d\n", MAJOR(device));

	return 0;

	error:
	kfree(&cdp);
	unregister_chrdev_region(device, ECHO_DEVS);
	return-1;

}



static void echo_exit(void)
{	
	int i;
	for(i=0;i<ECHO_DEVS;i++){
		release_region(cdp[i].Adr_base,1);
		cdev_del(&cdp[i].cdev);
	}
	kfree(cdp);
	unregister_chrdev_region(device, ECHO_DEVS);
	printk(KERN_ALERT "MAJOR DEVICE NUMBER %d is free now\n", MAJOR(device));
}

module_init(echo_init);
module_exit(echo_exit);
