
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
#include <linux/spinlock.h>
#include <linux/kfifo.h> 
#include <linux/interrupt.h>
#include <linux/wait.h>
#define COM1 0x3f8
#define MAX 100

DECLARE_WAIT_QUEUE_HEAD(myq1);
DECLARE_WAIT_QUEUE_HEAD(myq2);



typedef struct {
	struct cdev *cdev;
	//struct semaphore mutex;
	struct kfifo *rxfifo;
	dev_t dev;  
	char c;
	char *buff;
	int flag;
	int flagr;
	int sent;
	int size;
	//wait_queue_head_t rxwq; // for IH synchron.1

} seri_dev_t;
int init=1;

seri_dev_t *device; 

MODULE_LICENSE("Dual BSD/GPL");
char teste_buff[MAX];
ssize_t echo_write(struct file *filp, const char *buff, size_t count, loff_t *offp);
ssize_t echo_read (struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos);
int echo_open(struct inode *inode, struct file *filep);
int echo_release(struct inode *inodep, struct file *filep);

struct file_operations fops=
	{
		.owner= THIS_MODULE,
		.open= echo_open,
		.release= echo_release,
		.llseek=no_llseek,
		.read=echo_read,
		.write=echo_write,
		
	};

 irqreturn_t interrupt_RX( int irq, void *dev_id )
{
	seri_dev_t *devi;
	unsigned char IIR;
	unsigned char LSR;
	unsigned char t;
	int i;
	devi=dev_id;
	i=device->sent+1;
	IIR=inb(COM1+UART_IIR);
	
	printk ( KERN_ALERT "Hello Interrupt\n" );
	if(init==1)
		{
			
			init=0;
			printk(KERN_ALERT "\n Init %d \n",init);
			t=inb(COM1+UART_IER);
			printk(KERN_ALERT " \n Int IER %x\n",t);
			return IRQ_NONE;
		}
	else 

	if( IIR & UART_LSR_THRE ) 
		{
		if(i < device->size)
			{
				printk(KERN_ALERT " \n sent: %d    size: %d \n",device->sent,device->size);
				outb(device->buff[i],(COM1+UART_RX));
				device->sent++;
					if(device->sent+1==device->size)
						{
							printk(KERN_ALERT "\n CHEGOU \n");
							
						}
				return IRQ_HANDLED;
			}

		else 
			{
				printk(KERN_ALERT "\nAQUI\n");
				device->flagr=1;
				t=inb(COM1+UART_LSR);
				wake_up_interruptible(&myq2);				
				return IRQ_HANDLED;
			}
	}
	
	LSR=inb(COM1+UART_LSR);
	if( (LSR & UART_LSR_DR) )
		{
			//init = 1;
			device->c=(char)inb(COM1+UART_RX);	
			printk(KERN_ALERT"\n FUNC CHAR NA INTERRUPT: %c\n",device->c);	
			device->flag=1;
			wake_up_interruptible(&myq1);
			return IRQ_HANDLED;
		}
		
printk(KERN_ALERT "\n Algo falhou\n");		
	return IRQ_NONE;
}

int echo_open(struct inode *inode, struct file *filep)
{
	
	seri_dev_t *devi;
	int teste=0;
	unsigned char t;
	extern int nonseekable_open(struct inode * inode, struct file * filp);
	device->flagr=0;
	device->flag=0;
	device->sent=0;
	
	
	devi =container_of(inode->i_cdev, seri_dev_t, cdev);
	
	devi->flag=0;	
			
		if(teste<0)			
			{
				printk(KERN_ALERT "\n Erro no request_irq\n");
		}
		else if(teste==0)
				printk(KERN_ALERT "\n Request_irq com sucesso\n");
	
	
	filep->private_data=devi;

	printk(KERN_ALERT "\nFuncao open\n");
	t=inb(COM1+UART_IER);
	printk(KERN_ALERT " \nOPEN IER %x\n",t);

	return 0;

}

ssize_t echo_read (struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	
	//seri_dev_t *devi = filp->private_data;
	unsigned long ret;
	unsigned char t;
	//unsigned char LSR;
	printk(KERN_ALERT "\nFuncao Read   \n");
	t=inb(COM1+UART_IER);
	printk(KERN_ALERT " \nREAD IER %x\n",t);
	wait_event_interruptible(myq1, device->flag==1);
	device->flag=0;
	//LSR=inb(COM1+UART_LSR);
		
		
		
	
	/*while( !( LSR & (UART_LSR_DR | UART_LSR_OE | UART_LSR_PE | UART_LSR_FE) ) )
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
	*/
	
	//__kfifo_get(dev->rxfifo,&c,sizeof(char));
	
	printk(KERN_ALERT"\n FUNC CHAR: %c\n",device->c);
	
	ret=copy_to_user(buf,&device->c,count);
	
	return ret;	
	
}


ssize_t echo_write(struct file *filp, const char *buff, size_t count, loff_t *offp)
{
	unsigned long ret;
	device->size=sizeof(char)*count;
	device->buff= (char*)kmalloc(count,GFP_KERNEL);
	printk(KERN_ALERT "\nFuncao Write   \n");
	
	ret=copy_from_user(device->buff,buff,count);
	printk(KERN_ALERT "\nFuncao Write : %s   \n",device->buff);
	outb(device->buff[0],(COM1+UART_RX));

	wait_event_interruptible(myq2, device->flagr==1);
	device->flagr=0;
	/*printk(KERN_ALERT "\n str : %s \n",teste_buff);	
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
		
	
	
		while( !(LSR & UART_LSR_THRE) )
		{
			schedule();
			LSR=inb(COM1+UART_LSR);
		
		outb(teste_buff[i],(COM1+UART_RX));	
		LSR=inb(COM1+UART_LSR);
			
		}*/	
	
	printk(KERN_ALERT "\nFuncao Write finito   \n");
	kfree(device->buff);	
	return ret;

}

int echo_release(struct inode *inodep, struct file *filep)
{
	
	printk(KERN_ALERT "\nFuncao release\n");
	return 0;

}
/*void serial_setup(void){
	unsigned char ier = 0;
	unsigned char lcr = 0;
	unsigned char dll = 0;
	unsigned char dlm = 0;
	
	
	
	lcr |= UART_LCR_WLEN8 | UART_LCR_STOP | UART_LCR_EPAR | UART_LCR_PARITY | UART_LCR_DLAB;
	dll |= UART_DIV_1200;
	ier |= UART_IER_RDI | UART_IER_THRI;
	outb(lcr,COM1+UART_LCR);
	outb(dll,COM1+UART_DLL);
	outb(dlm,COM1+UART_DLM);
	lcr &= ~UART_LCR_DLAB;
	outb(lcr,COM1+UART_LCR);
	outb(ier,COM1+UART_IER);

}*/

static int echo_init(void)
{

	
	//struct kfifo *kfi;
	
	unsigned char LCR;
	unsigned char DL;
	unsigned char IER;
	unsigned char t;
	int major;
	int teste=1;
	int minor;	
	spinlock_t my_lock = SPIN_LOCK_UNLOCKED;
	char *str = (char*) kmalloc ( sizeof(char) * 16, GFP_KERNEL);
	request_region(0x3f8, 8,"seri2");
	device = kmalloc(sizeof(seri_dev_t),GFP_KERNEL);	

	
	teste=alloc_chrdev_region(&device->dev,0,1,"seri2");	
		
		if(teste<0)
			{
				printk(KERN_ALERT " \nErro no alloc_chrdev\n");
				return 0;		
			}
	
	major=MAJOR(device->dev);	
	minor=MINOR(device->dev);
	
	//cdev_init(device->cdev, &fops);
	device->cdev=cdev_alloc();
	(device->cdev)->owner = THIS_MODULE;
	(device->cdev)->ops = &fops;
	

	
	
	
	device->rxfifo=kfifo_init(str, sizeof(char)*16,GFP_KERNEL, &my_lock);
	

	teste= cdev_add(device->cdev,device->dev,1);
			
		if(teste<0)
			{
				printk(KERN_ALERT " \nErro no add\n");
				return 0;		
			}
	teste = request_irq(4,interrupt_RX, 0 ,"seri2",device);
	/*teste = request_irq(4,interrupt_RX, GFP_KERNEL ,"seri2",NULL);	
			
		if(teste<0)			
			{
				printk(KERN_ALERT "\n Erro no request_irq\n");
			}	*/
	printk(KERN_ALERT "Majjor: %d\n",major);
	
	LCR = UART_LCR_WLEN8 | UART_LCR_EPAR | UART_LCR_DLAB ;
	DL = UART_DIV_1200 | UART_DLM;
	IER = UART_IER_RDI | UART_IER_THRI | UART_IER_RLSI;

	outb(LCR,COM1+UART_LCR);
	outb(DL,COM1+UART_DLM);

	LCR = ( (UART_LCR_WLEN8) | (UART_LCR_EPAR) );
	
	outb(LCR,COM1+UART_LCR);	
	outb(IER,COM1+UART_IER);
	t=inb(COM1+UART_IER);
	//serial_setup();
	printk(KERN_ALERT " \nIER %x\n",t);
	//wait_event_interruptible(myq, device->flagr==1);
	return 0;
}

static void echo_exit(void)
{

	printk(KERN_ALERT "\nLimpou major: %d\n",MAJOR(device->dev));	
	printk(KERN_ALERT "\nLimpou major2: %d\n",MAJOR(device->dev));
	free_irq(4,device);
	kfifo_free(device->rxfifo);
	cdev_del(device->cdev);
	unregister_chrdev_region(device->dev,1);
	
	kfree(device);
	release_region(0x3f8, 8);
	
}

module_init(echo_init);
module_exit(echo_exit);
