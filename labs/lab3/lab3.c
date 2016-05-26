//Basic libs for kernel modules
#include <linux/init.h>
#include <linux/module.h>
//For copy_to_user() and copy_from_user() functions
#include <asm/uaccess.h>
//For dev_t type
#include <linux/types.h>
//For '*_chrdev_region' functions, 'file_operations' struct and 'file' struct
#include <linux/fs.h>
//For outb() and inb()
#include <asm/io.h>
//For cdev functions
#include <linux/cdev.h>
//For kmalloc() function
#include <linux/slab.h>
//For request_region() and release_region()
#include <linux/ioport.h>

#include <linux/jiffies.h>
#include <linux/sched.h>

#include "serial_reg.h"

MODULE_LICENSE("Dual BSD/GPL");

//UART register use the range 0x3f8-0x3ff (8 registers)
#define COM1 0x3f8 //baseaddr

dev_t dev;

/* Estrutura das funções implementadas pelo Driver */
struct file_operations fops=
{
	.owner = THIS_MODULE,
	.open = echo_open,
	.release = echo_release,
	.llseek = no_llseek, //Non seekable device
	.write = echo_write,
	.read = echo_read,
};

/*-------Funções------*/
static int serp_init(void); //Função de inicialização do device
static void serp_exit(void); //Função de terminação do device
static void serp_setup_dev(struct serp_d evices *device, int index); //Função de configuração/setup do módulo

int serp_open(struct inode *inode, struct file *filp); //Função open do device
int serp_release(struct inode *inode, struct file *filp); //Função de release do device

ssize_t serp_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos);
ssize_t serp_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp);
/*--------------------*/

static int serp_init(void) {

	//unsigned char LCR = UART_LCR_WLEN8 | UART_LCR_EPAR | UART_LCR_DLAB; //Control Byte
	//unsigned char DL = UART_DIV_1200 | UART_DLM;

    int i;
    int status = 0;
    unsigned char lcr = 0; //isto é o control byte
    major = 0; //Alocar dinamicamente
    
    printk(KERN_ALERT "Echo, world\n");
    
    /*Configurar UART: 8-bit chars; 2 stop bits; parity even; 1200 bps; NO Interrupts */
    lcr = UART_LCR_WLEN8 | UART_LCR_PARITY | UART_LCR_EPAR | UART_LCR_STOP;
    outb(LCR, COM1 + UART_LCR);
    
    lcr = lcr | UART_LCR_DLAB; // activar o acesso ao registo Divisor Latch
    outb(lcr, COM1 + UART_LCR);
    
    //Configurar a velocidade (bitrate)
    outb(UART_DIV_1200, COM1+UART_DLL);
    outb(0, COM1+UART_DLM);

    lcr &= ~UART_LCR_DLAB; // desactivar o acesso ao registo Divisor Latch
    outb(lcr, COM1 + UART_LCR);
    /*--------END-UART-----------*/
    
    /* Alocar o Device e o seu Major */
    status = alloc_chrdev_region(&dev, baseMinor, serpMinors, "serp");

    if (status < 0) {
        printk(KERN_INFO "Major number allocation failed \n");
        return status;
    } else {
        major = MAJOR(dev);
        minor = MINOR(dev);
        printk(d " The major number is %d \n", major);
        printk(KERN_INFO " The minor number is %d \n", minor);

        /* Aloca espaço no kernel para armazenar a estrutura char device */
        serpDevs = kmalloc(serpMinors * sizeof (struct serp_devices), GFP_KERNEL);


        for (i = 0; i < serpMinors; i++) {
            
            
            cdev_init(&serpDevs[i].serpCharDev, &serpFops);
            
            serpDevs[i].serpCharDev.ops = &serpFops;
           
            serpDevs[i].serpCharDev.owner = THIS_MODULE;

            status = 0;
            status = cdev_add(&serpDevs[i].serpCharDev, MKDEV(major, i), 1);
            if (status < 0) {
                printk(KERN_INFO "ERRO a adicionar o device ao kernel \n");
            } else {
                printk(KERN_INFO "Adicionado DEV #%d com minor = %d \n", MKDEV(major, i), i);
            }

        }

    }

    /*Configuração da UART - reservar o I/O port range */
    /*Mas atenção que temos que utilizar o disableserial.sh porque este range já se encontra atribuido por defeito */
    request_region(COM1, 8, "UART");
    
    return 0;
}