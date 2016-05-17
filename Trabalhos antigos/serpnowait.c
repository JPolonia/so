
/*                                                     
 * $Id: hello.c,v 1.5 2004/10/26 03:32:21 corbet Exp $ 
 */                                                    
#include <linux/init.h>
#include <linux/module.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>	
#include <linux/slab.h>		
#include <linux/fs.h>		
#include <linux/errno.h>	
#include <linux/types.h>	
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	
#include <linux/aio.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include "serial_reg.h"
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/ioport.h>

#define COM1 0x3f8
#define MAX 100





unsigned char LCR = UART_LCR_WLEN8 | UART_LCR_EPAR | UART_LCR_DLAB;
unsigned char DL = UART_DIV_1200 | UART_DLM;
unsigned char IER = 0;
dev_t dev;
char c='n';








MODULE_LICENSE("Dual BSD/GPL");
char teste_buff[MAX];

struct cdev * file_prop;
int major;
char *str_teste="lalala";
int bytes;
int minor;
ssize_t echo_write(struct file *filp, const char *buff, size_t count, loff_t *offp);
ssize_t echo_read(struct file *filp, const char *buff, size_t count, loff_t *offp);
int echo_open(struct inode *inode, struct file *filep);
int echo_release(struct inode *inodep, struct file *filep);

struct file_operations fops=
	{
		.owner= THIS_MODULE,
		.open= echo_open,
		.release= echo_release,
		.llseek=no_llseek,
		.write=echo_write,
		.read=echo_read,
	};

int echo_open(struct inode *inode, struct file *filep)
{
	
	
	
	extern int nonseekable_open(struct inode * inode, struct file * filp);
	filep->private_data=file_prop;
	printk(KERN_ALERT "\nFuncao open\n");
	return 0;

}

ssize_t echo_read(struct file *filp, const char *buff, size_t count, loff_t *offp)
{
	unsigned char LSR;
	char c;

	LSR=inb(COM1+UART_LSR);
		
	printk(KERN_ALERT "\nFuncao Read  %x \n",LSR);	
		
	
	while( !( LSR & (UART_LSR_DR | UART_LSR_OE | UART_LSR_PE | UART_LSR_FE) ) )
		{
			//printk(KERN_ALERT " \n Nabooooooooooooooo %x \n",LSR);
			schedule();
			LSR=inb(COM1+UART_LSR);			
			
		}
			printk(KERN_ALERT "\nFuncao Read2  %x \n",LSR);	
		
			if ( LSR & UART_LSR_OE )
				{
					printk(KERN_ALERT "\nOverrun\n");
					return -EIO;				
				}
			else			
				if (LSR & UART_LSR_PE)
				{
					printk(KERN_ALERT "\nParity error\n");
					return -EIO;
				}
				
			else			
				if (LSR & UART_LSR_FE)
				{
					printk(KERN_ALERT "\nFrame error\n");
					return -EIO;
				}
	
	c=(char)inb(COM1+UART_RX);
	
	printk(KERN_ALERT"\n FUNC CHAR: %c\n",c);
	
	copy_to_user(buff,&c,count);

	return 1;	
	
}


ssize_t echo_write(struct file *filp, const char *buff, size_t count, loff_t *offp)
{
	unsigned char LSR;
	int i=0;
	char *teste_buff= (char*)kmalloc(count,GFP_KERNEL);
	printk(KERN_ALERT "\nFuncao Write   \n");
	if(count>MAX)
		count=MAX;
	copy_from_user(teste_buff,buff,count);
	printk(KERN_ALERT "\n str : %s \n",teste_buff);	
	LSR=inb(COM1+UART_LSR);
	//for(i=0;i<count;i++)
	while(i<count)	
	{
		
		if( !(LSR & UART_LSR_THRE) )
			schedule();
		else 
			{
				outb(teste_buff[i],(COM1+UART_RX));
				i++;		
			}
		LSR=inb(COM1+UART_LSR);
		
	
	
		/*while( !(LSR & UART_LSR_THRE) )
		{
			schedule();
			LSR=inb(COM1+UART_LSR);
		}
		outb(teste_buff[i],(COM1+UART_RX));	
		LSR=inb(COM1+UART_LSR);
			*/
		}	
	
	printk(KERN_ALERT "\nFuncao Write finito   \n");
	kfree(teste_buff);	
	return count;

}

int echo_release(struct inode *inodep, struct file *filep)
{
	
	printk(KERN_ALERT "\nFuncao release\n");
	return 0;

}

static int echo_init(void)
{

	request_region(0x3f8, 8,"serpnowait");
	outb(LCR,COM1+UART_LCR);
	outb(DL,COM1+UART_DLM);
	LCR =( (UART_LCR_WLEN8) | (UART_LCR_EPAR) );
	outb(LCR,COM1+UART_LCR);
	outb(IER,COM1+UART_IER);
	
	int teste=1;
	teste=alloc_chrdev_region(&dev,0,1,"serpnowait");	
		if(teste<0)
			{
				printk(KERN_ALERT " \nErro no alloc_chrdev\n");
				return 0;		
			}
	
	major=MAJOR(dev);
	minor=MINOR(dev);
	file_prop = cdev_alloc();
	file_prop->owner = THIS_MODULE;
	file_prop->ops = &fops;
	teste= cdev_add(file_prop,dev,1);
	
			if(teste<0)
			{
				printk(KERN_ALERT " \nErro no add\n");
				return 0;		
			}
			
	printk(KERN_ALERT "Majjor: %d\n",major);

	return 0;
}

static void echo_exit(void)
{

	printk(KERN_ALERT "\nLimpou major: %d\n",MAJOR(dev));	
	printk(KERN_ALERT "\nLimpou major2: %d\n",MAJOR(dev));
	cdev_del(file_prop);
	unregister_chrdev_region(dev,1);
	release_region(0x3f8, 8);
	
}

module_init(echo_init);
module_exit(echo_exit);
