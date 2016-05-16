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

#include "serp.h"
#include "serial_reg.h"

MODULE_LICENSE("Dual BSD/GPL");

dev_t dev;
int major = 0, minor = 0;
int nchars = 0;

//UART register use the range 0x3f8-0x3ff (8 registers)
#define baseaddr 0x3f8


/*
struct serp_devices{  >>> ESTÁ NO .H
  struct cdev serpCharDev; //Estrutura no kernel que representa o char device 
};
 */

/*
 * O valor do major definido no serp.h é 0, o que é de facto inválido, e faz com que o SO atribua um 
 * major livre, de forma dinâmica.
 */
int serpMajor = ECHO_MAJOR;

/*Representa o número de Devices que vão na prática existir*/
int serpMinors = ECHO_DEVS;

/* Representa o minor base */
int baseMinor = 0;


static int serp_init(void); //Função de inicialização do device
static void serp_exit(void); //Função de terminação do device
static void serp_setup_dev(struct serp_devices *device, int index); //Função de configuração/setup do módulo

int serp_open(struct inode *inode, struct file *filp); //Função open do device
int serp_release(struct inode *inode, struct file *filp); //Função de release do device

ssize_t serp_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos);
ssize_t serp_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp);


struct serp_devices *serpDevs;

/* Estrutura das funções implementadas pelo Driver */
struct file_operations serpFops = {
    .owner = THIS_MODULE,
    .llseek = no_llseek, //Non seekable device
    .open = serp_open,
    .read = serp_read,
    .write = serp_write,
    .release = serp_release,
    //.ioctl = uart_ioctl,
};

static int serp_init(void) {

    int i, ret = 0;
    unsigned char lcr = 0; //isto é o control byte
    
    printk(KERN_ALERT "Echo, world\n");
    
    /*Configurar UART */
    /*
        * 8-bit chars, 2 stop bits, parity even, and 1200 bps. 
       *  Because, we will not use interrupts, make sure that the UART is configured not to generate interrupts
        */
    lcr = UART_LCR_WLEN8 | UART_LCR_PARITY | UART_LCR_EPAR | UART_LCR_STOP;
    outb(lcr, baseaddr + UART_LCR);
    
    lcr = lcr | UART_LCR_DLAB; // activar o acesso ao registo Divisor Latch
    outb(lcr, baseaddr + UART_LCR);
    
    //Configurar a velocidade (bitrate)
    outb(UART_DIV_1200, baseaddr+UART_DLL);
    outb(0, baseaddr+UART_DLM);

    lcr &= ~UART_LCR_DLAB; // desactivar o acesso ao registo Divisor Latch
    outb(lcr, baseaddr + UART_LCR);
    
    
    /* Alocar o Device e o seu Major */
    ret = alloc_chrdev_region(&dev, baseMinor, serpMinors, "serp");

    if (ret < 0) {
        printk(KERN_INFO "Major number allocation failed \n");
        return ret;
    } else {
        major = MAJOR(dev);
        minor = MINOR(dev);
        printk(KERN_INFO " The major number is %d \n", major);
        printk(KERN_INFO " The minor number is %d \n", minor);

        /* Aloca espaço no kernel para armazenar a estrutura char device */
        serpDevs = kmalloc(serpMinors * sizeof (struct serp_devices), GFP_KERNEL);


        for (i = 0; i < serpMinors; i++) {
            
            
            cdev_init(&serpDevs[i].serpCharDev, &serpFops);
            
            serpDevs[i].serpCharDev.ops = &serpFops;
           
            serpDevs[i].serpCharDev.owner = THIS_MODULE;

            ret = 0;
            ret = cdev_add(&serpDevs[i].serpCharDev, MKDEV(major, i), 1);
            if (ret < 0) {
                printk(KERN_INFO "ERRO a adicionar o device ao kernel \n");
            } else {
                printk(KERN_INFO "Adicionado DEV #%d com minor = %d \n", MKDEV(major, i), i);
            }

        }

    }

    /*Configuração da UART - reservar o I/O port range */
    /*Mas atenção que temos que utilizar o disableserial.sh porque este range já se encontra atribuido por defeito */
    request_region(baseaddr, 8, "UART");
    
    return 0;
}



/**
 *  OPEN
 */
int serp_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Inside echo_open\n");
    
    
    struct serp_dev *sdevp;
    
    /*Verificar retorno depois - para já serve para informar o kernel de que o device é non seekable*/
    nonseekable_open(inodep, filep);
      
    sdevp = container_of(inodep->i_cdev, struct serp_devices, serpCharDev);
    filep->private_data = sdevp;
    
    
    printk(KERN_ALERT "serp_open() \n");
    
    return 0;
}



/*
 * WRITE
 */
ssize_t serp_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp){
    
  printk(KERN_INFO "Inside serp write \n");
  
  int i=0;
  ssize_t sent=0; 
  unsigned long error;
  
  /*Alocar espaço em kernel level com tamanho para a mensagem a escrever = count*/
  char *echoKernelBuf;
  echoKernelBuf = kzalloc(count * sizeof(char)+1, GFP_KERNEL); // tem que ter espaço para o \0
  
  //ENOMEM = Not enough space
  if(!echoKernelBuf) return -ENOMEM;
  
  error = copy_from_user(echoKernelBuf, buff, count);
  

    //Tratamento de erros que podem ocorrer 
    if (error == 0) {
        printk(KERN_INFO "Escreveu tudo = %d \n", error);
        printk(KERN_INFO "%s \n", echoKernelBuf);
        //return 0; //o objectivo é que retorne o numero de caracteres que escreveu
    } else
        if (error > 0) {
        printk(KERN_INFO "Falta escrever %lu \n", error);    
    }

  
  //echoKernelBuf[count] = '\0';
  
  for(i = 0; i<strlen(echoKernelBuf); i++)
  {
    //while(1){  
      while( ( ( inb(baseaddr+UART_LSR) ) & UART_LSR_THRE) == 0 )
      {
           schedule(); // se o LSR nao estiver vazio o driver tem de esperar (aqui ele deveria largar o CPU e não fazer busy waiting) e é para isso que serve esta funçao
      }
      
       outb(echoKernelBuf[i], ( baseaddr + UART_TX) );
       sent++; //acumulador do número de caracteres enviados
       //i++;
       
       //if(i==count) 
           //break;
     //}
  }
 
  kfree(echoKernelBuf);
  printk(KERN_INFO "Write: escreveu %d \n", sent);

  //return (count - error); //assim retorna o número de caracteres que faltam escrever
  return sent; // retorna o número de caracteres efectivamente escritos;
}

/*
 * READ
 */
ssize_t serp_read(struct file *filep, char __user *buff, size_t count, loff_t *offp) {

    printk(KERN_INFO "\n\n Inside serp read \n");
    
    int i = 0;
    ssize_t received = 0;
    unsigned long error;

    /*Alocar espaço em kernel level com tamanho para a mensagem a escrever = count*/
    char *echoKernelBuf = (char *) kzalloc(count * sizeof (char) + 1, GFP_KERNEL); // tem que ter espaço para o \0
    
    //ENOMEM = Not enough space
     if(!echoKernelBuf) return -ENOMEM;

    while (1) {
        if ((inb(baseaddr + UART_LSR) & (UART_LSR_FE | UART_LSR_OE | UART_LSR_PE)) != 0) {
            printk(KERN_ALERT "Erro no read \n");
            return -EIO;

        }
        else if ((inb(baseaddr + UART_LSR) & UART_LSR_DR) == 0) {
            //ainda nao há dados para receber... tem de esperar
            //printk(KERN_ALERT "read: à espera... \n");
            schedule();
        } else {
            // é porque já há dados a receber... por isso já se podem copiar para o buffer
            echoKernelBuf[i] = inb(baseaddr + UART_RX);
            
            if(echoKernelBuf[i] == 10 || i==count)
            {
                echoKernelBuf[i] = '\0'; 
                break;
            }
            
            i++;
            received++;
           
        }
    }

    
    error = copy_to_user(buff,echoKernelBuf, received);
    
        if (error == 0) {
        printk(KERN_INFO "Leu %d \n", received);
        //printk(KERN_INFO "%s \n", buff);
    } else
        if (error > 0) {
        printk(KERN_INFO "Falta ler %d \n", count - (int)received );
    }

    kfree(echoKernelBuf);
    
    return received; // retorna o número de caracteres efectivamente recebidos;
    }



/*
 * RELEASE
 */
int serp_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Inside echo release \n");
    return 0;
}


/*
 * EXIT
 */
static void serp_exit(void) {
    int i;
    
    for(i=0; i<serpMinors; i++){
        cdev_del(&serpDevs[i].serpCharDev);
        printk(KERN_INFO "Removido device minor # %d \n",i);
    }
    
    kfree(serpDevs);
    
    unregister_chrdev_region(MKDEV(major, 0), serpMinors);
    outb(0x00, baseaddr+UART_LCR); // limpar configurações
    
    //libertar a região previamente reservada no procedimento de init()
    release_region(baseaddr, 8);
    
    printk(KERN_INFO " Goodbye:  major number is %d \n", major);
    
 
}





module_init(serp_init);
module_exit(serp_exit);
