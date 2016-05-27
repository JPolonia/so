#include <linux/init.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/slab.h>
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
#define COM1 0x3f8

int serpMajor = ECHO_MAJOR;

/*Representa o número de Devices que vão na prática existir*/
int serp_nr_devs = ECHO_DEVS;

/* Representa o minor base */
int serpMinor = 0;


static int serp_init(void); //Função de inicialização do device
static void serp_exit(void); //Função de terminação do device
//static void serp_setup_dev(struct serp_devices *device, int index); //Função de configuração/setup do módulo

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
};

static int serp_init(void) {

    int i, result = 0;
    unsigned char lcr = 0; //isto é o control byte
    
    printk(KERN_ALERT "%-10sSerial Port Device Driver - Polled Operation \n\n","INIT:");
    
    /*----------UART-CONFIGURATION--------*/    
    /*Configurar UART |8-bit chars, 2 stop bits, parity even, and 1200 bps|Because, we will not use interrupts, make sure that the UART is configured not to generate interrupts*/
    lcr = UART_LCR_WLEN8 | UART_LCR_PARITY | UART_LCR_EPAR | UART_LCR_STOP;
    outb(lcr, COM1 + UART_LCR);
    
    lcr = lcr | UART_LCR_DLAB; // activar o acesso ao registo Divisor Latch
    outb(lcr, COM1 + UART_LCR);
    
    //Configurar a velocidade (bitrate)
    outb(UART_DIV_1200, COM1+UART_DLL);
    outb(0, COM1+UART_DLM);

    lcr &= ~UART_LCR_DLAB; // desactivar o acesso ao registo Divisor Latch
    outb(lcr, COM1 + UART_LCR);

    major = ECHO_MAJOR;
    minor = serpMinor;
    /*--------------END-UART---------------*/

    /*------ALLOCATING-DEVICE-NUMBERS-----*/
    if (ECHO_MAJOR) { //Se estiver definido 
        printk(KERN_INFO "%-10sMajor is defined by user: %d \n","INFO:",ECHO_MAJOR);
        dev = MKDEV(ECHO_MAJOR,serpMinor);
        result = register_chrdev_region(dev,serp_nr_devs,"serp");
    } else {
        printk(KERN_INFO "%-10sMajor is dynamically set \n","INFO:");
        result = alloc_chrdev_region(&dev, serpMinor, serp_nr_devs,"serp");
        
    }

    if (result < 0) {
        printk(KERN_WARNING "%-10sMajor number allocation failed: %d\n","ERROR:", major);
        return result;
    }         

    major = MAJOR(dev);
    minor = MINOR(dev);

    printk(KERN_INFO "%-10sDevice Number Allocated!! \n","SUCCESS:");
    printk(KERN_INFO "%-10sMajor: %d \n", " ",major);
    printk(KERN_INFO "%-10sMinor: %d \n\n", " ",minor);
    /*----------------END-----------------*/



    /*---ALLOCATING-CHAR-DEVICE-STRUCT----*/
    serpDevs = kmalloc(serp_nr_devs * sizeof (struct serp_devices), GFP_KERNEL);

    printk(KERN_INFO "%-10sAllocating %d devices (User defined)\n","INFO:",serp_nr_devs);

    for (i = 0; i < serp_nr_devs; i++) {
        cdev_init(&serpDevs[i].serpCharDev, &serpFops);
        
        serpDevs[i].serpCharDev.ops = &serpFops;
       
        serpDevs[i].serpCharDev.owner = THIS_MODULE;

        result = 0;
        result = cdev_add(&serpDevs[i].serpCharDev, MKDEV(major, i), 1);
        if (result < 0)
            printk(KERN_INFO "%-10sCould not add DEV: %d with minor: \n","ERROR:",MKDEV(major, i), i);
        else
            printk(KERN_INFO "%-10sAdded new DEV: #%d  Minor: %d \n","", MKDEV(major, i), i);
    }
    /*----------------END-----------------*/
      

    /*Configuração da UART - reservar o I/O port range - USAR DISABLESERIAL*/
    result = request_region(COM1, 8, "UART");
    if (!result){
        printk(KERN_WARNING "%-10sRequest region failed! Please run disableserial.sh...","ERROR:");
        return result;
    }

    
    //SUCCESS!
    printk(KERN_INFO "\n%-10sSERP INITIALIZED!! \n\n","SUCCESS:");
    return 0;
}

static void serp_exit(void) {
    int i;

    printk(KERN_ALERT "%-10sSerial Port Device Driver - Polled Operation \n\n","EXIT:");
    
    printk(KERN_INFO "%-10sRemoving %d devices (User defined)\n","INFO:",serp_nr_devs);
    for(i=0; i<serp_nr_devs; i++){
        cdev_del(&serpDevs[i].serpCharDev);
        printk(KERN_INFO "%-10sRemoved DEV: #%d  Minor: %d \n","", MKDEV(major, i), i);
    }
    
    kfree(serpDevs);
    
    unregister_chrdev_region(MKDEV(major, 0), serp_nr_devs);
    outb(0x00, COM1+UART_LCR); // limpar configurações
    
    //libertar a região previamente reservada no procedimento de init()
    release_region(COM1, 8);
    
    printk(KERN_INFO "\n%-10sSERP REMOVED!! \n\n","SUCCESS:");
}


int serp_open(struct inode *inodep, struct file *filep) {
    printk(KERN_ALERT "%-10sOpening file... ","OPEN:");

    struct serp_dev *serp_dev_open;
    
    nonseekable_open(inodep, filep);
      
    serp_dev_open = container_of(inodep->i_cdev, struct serp_devices, serpCharDev);
    filep->private_data = serp_dev_open;
    
    printk(KERN_INFO " OK! \n\n");
    
    return 0;
}

int serp_release(struct inode *inodep, struct file *filep) {
    printk(KERN_ALERT "%-10sReleasing file... ","RELEASE:");
    printk(KERN_INFO " OK! \n\n");
    return 0;
}

ssize_t serp_read(struct file *filep, char __user *buff, size_t count, loff_t *offp) { 
    int index = 0;
    ssize_t received = 0;
    unsigned long result;
    unsigned char var;
    unsigned short int LSR_adress, RX_adress;
    char *buff_kernel;

    printk(KERN_ALERT "%-10sReading file... \n\n","READ:");

    /*Alocar espaço em kernel level com tamanho para a mensagem a escrever = count*/
    buff_kernel = (char *) kzalloc(count * sizeof (char) + 1, GFP_KERNEL);
    if(!buff_kernel) 
      return -ENOMEM;
    
    /*Adress for receive buffer and line status register */
    RX_adress = COM1 + UART_RX;
    LSR_adress = COM1 + UART_LSR;
    

    while (1) {
        var = inb(LSR_adress);

        //Check for LSR errors (Frame, parity and overrun erros)
        if (var & (UART_LSR_FE | UART_LSR_OE | UART_LSR_PE)) { 
            printk(KERN_WARNING "%-10sFrame/Parity/Overrun error indicator %d\n","ERROR:");
            return -EIO;

        } else if (!(var & UART_LSR_DR)){ //Check if data is not ready and schule to release CPU
            schedule();
        } else {
            //Starts Reading...
            buff_kernel[index] = inb(RX_adress);
            
            if(buff_kernel[index] == 10 || index==count){ 
                buff_kernel[index] = '\0'; 
                break;
            }
            
            index++;
            received++;
           
        }
    }

    
    result = copy_to_user(buff,buff_kernel, received);
    
    if (!result) //All chars received
        printk(KERN_INFO "\n%-10sReceived %d chars: %s \n\n","SUCCESS:",received,buff);

    if (result > 0)
      printk(KERN_WARNING "\n%-10sMissing %d chars\n\n","WARNING:",count - (int)received);


    kfree(buff_kernel);
    
    return received;
}

ssize_t serp_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp){
    
  printk(KERN_ALERT "%-10sWriting file... \n\n","WRITE:");
  
  int i= 0;
  ssize_t sent=0; 
  unsigned long result;
  unsigned char var;
  unsigned short int LSR_adress, TX_adress;
  char *buff_kernel;
  
  /*Alocar espaço em kernel level com tamanho para a mensagem a escrever = count*/
  buff_kernel = kzalloc(count * sizeof(char)+1, GFP_KERNEL);
  if(!buff_kernel) 
    return -ENOMEM;

  /*Adress for transmitter buffer and line status register */
  TX_adress = COM1 + UART_TX;
  LSR_adress = COM1 + UART_LSR; 
  
  for(i = 0; i<count; i++){
      var = inb(LSR_adress);
      while(!(var & UART_LSR_THRE) )//Transmit hold register empty... Schedule to release CPU
           schedule(); 
      
       outb(buff_kernel[i],(TX_adress));
       sent++; 
  }
 
  kfree(buff_kernel);

  result = copy_from_user(buff_kernel, buff, count);
  
  //Tratamento de erros que podem ocorrer 
  if (!result) 
      printk(KERN_INFO "\n%-10sSent all %d chars: %s \n\n","SUCCESS:",sent,buff_kernel);

  if (result > 0) 
      printk(KERN_WARNING "\n%-10sMissing %d chars\n\n","WARNING:",count - result);   

  //return (count - error); //assim retorna o número de caracteres que faltam escrever
  return sent; // retorna o número de caracteres efectivamente escritos;
}

module_init(serp_init);
module_exit(serp_exit);
