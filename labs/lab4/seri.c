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
#include "seri.h"
#include "serial_reg.h"
#include "kfifo.h"


MODULE_LICENSE("Dual BSD/GPL");

int echo_open(struct inode *inodep, struct file *filep);
int echo_release(struct inode *inodep, struct file *filep);
ssize_t echo_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp);
ssize_t echo_read(struct file *filep, char __user *buff, size_t count, loff_t *offp);
irqreturn_t short_interrupt(int irq, void *dev_id);

struct access_control{
	spinlock_t lock;
	int count;
	int uid;
};

struct echo_dev{
	struct cdev cdev;
	unsigned short int Adr_base;
	int cnt;
	struct kfifo *RX, *TX;
	struct semaphore S_RX,S_TX;
	struct access_control A_control;
	wait_queue_head_t wait_RX;
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

irqreturn_t short_interrupt(int irq, void *dev_id){
	unsigned char iir, chr;
	struct echo_dev *edevp;
	edevp = dev_id;
	iir = inb(edevp->Adr_base+UART_IIR);

	//data received
	if(iir & UART_IIR_RDI){
		
		chr = inb(edevp->Adr_base+UART_RX);
		if(kfifo_put(edevp->RX, &chr,1)!=1){
			 printk(KERN_ALERT "FIFO RX complet\n");
		}
		
		wake_up_interruptible(&edevp->wait_RX);
	}else if(iir & UART_IIR_THRI){// trasmiting ready
		if(kfifo_len(edevp->TX)){
			kfifo_get(edevp->TX,&chr, 1);
			outb(chr,edevp->Adr_base+UART_TX);
		}
	}

	return IRQ_NONE;
}


int echo_open(struct inode *inodep, struct file *filep) {
	struct echo_dev *edevp;
	spinlock_t *spin,*spin2;
	nonseekable_open(inodep,filep);
	edevp = container_of(inodep->i_cdev, struct echo_dev, cdev);
	filep->private_data = edevp;
	spin_lock(&edevp->A_control.lock);
	if(edevp->A_control.count && (edevp->A_control.uid!= current->uid) &&  (edevp->A_control.uid!= current->euid)&& !capable(CAP_DAC_OVERRIDE)){
		spin_unlock(&edevp->A_control.lock);
		return -EBUSY; 
	}
		
	if(edevp->A_control.count == 0){	
		edevp->A_control.uid= current->uid;
		if ( request_irq(IRQ,short_interrupt, IRQF_SAMPLE_RANDOM | IRQF_SHARED , "seri", edevp) != 0) {
	    		printk(KERN_ALERT "Error allocating interrupt\n");
			spin_unlock(&edevp->A_control.lock);
	    		return -EINTR;
	  	}
		spin = kmalloc(sizeof(spinlock_t),GFP_KERNEL);
		if(spin == NULL){ return -ENOMEM;}
		spin2 = kmalloc(sizeof(spinlock_t),GFP_KERNEL);
		if(spin2 == NULL){goto fim;}
		spin_lock_init(spin);
		spin_lock_init(spin2);
		
		edevp->RX = kfifo_alloc(FIFO_SIZE,GFP_KERNEL,spin);
		if(edevp->RX == NULL) goto fim2;
		edevp->TX = kfifo_alloc(FIFO_SIZE,GFP_KERNEL,spin2);
		if(edevp->TX == NULL) goto fim3;
		init_MUTEX(&edevp->S_RX);
		init_MUTEX(&edevp->S_TX);
		init_waitqueue_head(&edevp->wait_RX);
	 	// enable data ready, transmitter empty 
	  	outb(UART_IER_RDI | UART_IER_THRI, edevp->Adr_base+UART_IER);
		outb(UART_FCR_TRIGGER_4 | UART_FCR_ENABLE_FIFO, edevp-> Adr_base + UART_FCR);
	}
	edevp->A_control.count++;

	spin_unlock(&edevp->A_control.lock);
	printk(KERN_ALERT "\n echo_open call\n");
	return 0; 
	fim3:
	kfifo_free(edevp->RX);
	fim2:
	kfree(spin2);
	fim:
	kfree(spin); 
	return -ENOMEM;	
}

int echo_release(struct inode *inodep, struct file *filep) {
	struct echo_dev *edevp;
	edevp = filep->private_data;
	spin_lock(&edevp->A_control.lock);
	edevp->A_control.count--;
	if(edevp->A_control.count==0){
		while(kfifo_len(edevp->TX)!=0){
			msleep_interruptible(1);
		}
		kfree(edevp->RX->lock);
		kfree(edevp->TX->lock);
		kfifo_free(edevp->RX);
		kfifo_free(edevp->TX);

		//disable interrupts
		outb(0x00, edevp->Adr_base+UART_IER);
		free_irq(IRQ, edevp);
	}
	spin_unlock(&edevp->A_control.lock);
	printk(KERN_ALERT "\n echo_realease call\n");
	return 0;
}

ssize_t echo_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp){
	int i;
	char *buffer, chr;
	//unsigned short int Adr_lsr, Adr_tx;
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

	i=0;
	if (down_interruptible(&edevp->S_TX)){
		kfree(buffer);
		return -ERESTARTSYS;
	}
	do{
		i+= kfifo_put(edevp->TX,&buffer[i],count-i);
		if((inb(edevp->Adr_base + UART_LSR)& UART_LSR_THRE) && kfifo_len(edevp->TX)!=0){
			kfifo_get(edevp->TX,&chr, 1);
			outb(chr,edevp->Adr_base+UART_TX);	
		}
		if(i==count || (filep->f_flags & O_NONBLOCK)){
			break;
		}
		//sleep for a while
		
	}while(1);
	up(&edevp->S_TX);
	edevp->cnt+=count;
	kfree(buffer);
	return i;
}

ssize_t echo_read(struct file *filep, char __user *buff, size_t count, loff_t *offp){
	char* buffer;
	int i;
	struct echo_dev *edevp;
	if(count==0) return 0;
	edevp = (struct echo_dev *) filep->private_data;
	buffer = kmalloc(count*sizeof(char), GFP_KERNEL);
	if(buffer == NULL){
		return -1;
	}
	
	i=0;
	if (down_interruptible(&edevp->S_RX)){
		kfree(buffer);
		return -ERESTARTSYS;
	}
	do{
		i+= kfifo_get(edevp->RX,&buffer[i],count-i);
		if(i==count || (filep->f_flags & O_NONBLOCK)){
			break;
		}
		if(wait_event_interruptible_timeout(edevp->wait_RX, false,  HZ/2)!=0) break;
	}while(1);
	up(&edevp->S_RX);
	count=i;
	i = copy_to_user((void __user*) buff, (const void*) buffer, i);
	kfree(buffer);
	
	return count-i;
}


static int echo_init(void)
{
	int i;
	struct resource *port;
	// allocate Major Device
	if(alloc_chrdev_region(&device,0,ECHO_DEVS,"seri") < 0){
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
		spin_lock_init(&cdp[i].A_control.lock);
		cdp[i].A_control.count = 0;
		if((cdev_add(&cdp[i].cdev,device,1))<0){
			printk(KERN_ALERT "Error! cannot register device");
			goto error;			
		}
	}
	
	port = request_region(ADR_COM1,1,"seri");
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
