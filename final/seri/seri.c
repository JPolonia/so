#include <linux/init.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "seri.h"
#include "serial_reg.h"
#include "ioctl.h"

MODULE_LICENSE("Dual BSD/GPL");


static DECLARE_WAIT_QUEUE_HEAD(wq); // fila para assinalar interrupção por forma a permitir ao read dormir
static int flag = 0; //flag utilizada para acordar o read

dev_t dev;

/*Variáveis auxiliares para usar em printk*/
int major = 0, minor = 0;
int nchars = 0;

//UART register use the range 0x3f8-0x3ff (8 registers)
#define baseaddr 0x3f8

#define MAXSIZE 1000
#define CLOCKBASE 115200


/*
 * O valor do major definido no serp.h é 0, o que é de facto inválido, e faz com que o SO atribua um 
 * major livre, de forma dinâmica.
 */
int seriMajor = SERP_MAJOR;

/*Representa o número de Devices que vão na prática existir*/
int seriMinors = SERP_DEVS; //igual a 4

/* Representa o minor base */
int baseMinor = 0;

int increment = 0;

int counter;


static int seri_init(void); //Função de inicialização do device
static void seri_exit(void); //Função de terminação do device

int seri_open(struct inode *inode, struct file *filp); //Função open do device
int seri_release(struct inode *inode, struct file *filp); //Função de release do device

ssize_t seri_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos);
ssize_t seri_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp);

static int seri_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg); //IOCTL 

struct seri_devices *seriDevs;

static char *buffer; // é preciso retirar este buffer daqui e colocalo dentro da estrutura.


/* Estrutura das funções implementadas pelo Driver */
struct file_operations seriFops = {
    .owner = THIS_MODULE,
    .llseek = no_llseek, //Non seekable device
    .open = seri_open,
    .read = seri_read,
    .write = seri_write,
    .release = seri_release,
    .ioctl = seri_ioctl,
};




static int seri_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
    char auxVar[10];
    int err = 0;

    //verificação de erros
    //Ch. 6 of the LDD3 book, pp. 142
    if (_IOC_TYPE(cmd) != SERP_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > SERP_IOC_MAXNR)    return -ENOTTY;

    //verifica comando e retorna
    //Ch. 6 of the LDD3 book, pp. 143      
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user *) arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ, (void __user *) arg, _IOC_SIZE(cmd));
    if (err)
        return -EFAULT;

    switch (cmd) {
        case SERP_IOCGLCR:
            printk(KERN_NOTICE "\n");
            printk(KERN_NOTICE "Word length: %d bits.\n", (inb(baseaddr + UART_LCR) & 0x03) + 5);
            printk(KERN_NOTICE "Stop bit(s): %d bit(s).\n", ((inb(baseaddr + UART_LCR) & UART_LCR_STOP) >> 2) + 1);

            switch ((inb(baseaddr + UART_LCR) & 0x38) >> 3) {
                case 1:
                    strcpy(auxVar, "Odd");
                    break;

                case 3:
                    strcpy(auxVar, "Even");
                    break;

                case 5:
                    strcpy(auxVar, "1");
                    break;

                case 7:
                    strcpy(auxVar, "0");
                    break;

                default:
                    strcpy(auxVar, "None");
                    break;
            }

            printk(KERN_NOTICE "Parity: %s.\n", auxVar);
            printk(KERN_NOTICE "Break Enable: %s.\n", ((inb(baseaddr + UART_LCR) & UART_LCR_SBC) >> 6) == 1 ? "Enabled" : "Disabled");

            outb(inb(baseaddr + UART_LCR) | UART_LCR_DLAB, baseaddr + UART_LCR);
            printk(KERN_NOTICE "Baudrate: %d.\n", CLOCKBASE / ((inb(baseaddr + UART_DLL) & 0x00FF) | (inb(baseaddr + UART_DLM) << 8)));
            outb(inb(baseaddr + UART_LCR) & ~UART_LCR_DLAB, baseaddr + UART_LCR);
            break;

        case SERP_IOCSLCR:
            outb(arg, baseaddr + UART_LCR);

            printk(KERN_NOTICE "New conf!\n");
            seri_ioctl(inode, filp, SERP_IOCGLCR, 1);
            break;

        case SERP_IOCSBAUD:
            outb(inb(baseaddr + UART_LCR) | UART_LCR_DLAB, baseaddr + UART_LCR);
            outb((CLOCKBASE / arg) & 0x00FF, baseaddr + UART_DLL);
            outb(((CLOCKBASE / arg) & 0xFF00) >> 8, baseaddr + UART_DLM);
            outb(inb(baseaddr + UART_LCR) & ~UART_LCR_DLAB, baseaddr + UART_LCR);
            seri_ioctl(inode, filp, SERP_IOCGLCR, 1);
            break;

        default:
            return -ENOTTY;

    }
    return 0;
}


/*
 In Linux, an interrupt handler can be a C function with the following prototype:

#include <linux/interrupt.h>
irqreturn_t short_interrupt(int irq, void *dev_id)

So that the IH is run upon an interrupt generated by the serial port, the seri module must register it invoking the function:

#include <linux/interrupt.h>
int request_irq(unsigned int irq,   irqreturn_t (*handler)(int, void *),     unsigned long flags,     const char *dev_name, void *dev_id)
 
 * Tem 4 argumentos:
 *  unsigned int irq - interrupt request number line associated to the serial port = 4
 * 
 */

irqreturn_t interrupt_handler(int irq, void *dev_id) {
    unsigned char IIR, LSR;
    unsigned int i = 0, n;


    IIR = inb(baseaddr + UART_IIR);

    switch (IIR) {
        case UART_IIR_RLSI:
            printk(KERN_ALERT "LINE STATUS INTERRUPT");
            break;

        case UART_IIR_RDI:
            printk(KERN_ALERT "RDI");

            if (buffer != NULL) kfree(buffer);

            buffer[increment] = inb(baseaddr + UART_RX);
            //printk(KERN_ALERT "buffer[i] = %d \n", (int) buffer[increment]);
            increment++;
            flag = 1;
            wake_up_interruptible(&wq);

            break;




        case UART_IIR_THRI:
            printk(KERN_INFO "THRI");

            if (buffer == NULL) return IRQ_NONE;

            for (i = 0; i < MAXSIZE; i++) {

                outb(buffer[i], baseaddr + UART_TX);


                LSR = inb(baseaddr + UART_LSR);

                while (!(LSR & UART_LSR_THRE))
                    LSR = inb(baseaddr + UART_LSR);
            }

            kfree(buffer);
            buffer = NULL;
            break;
    }


    return IRQ_HANDLED;
}




static int seri_init(void) {

    int i, result = 0;
    unsigned char lcr = 0; //isto é o control byte
    
    printk(KERN_ALERT "%-10sSerial Port Device Driver - Interrupt-driven Operation \n\n","INIT:");
    
    /*Configurar UART |8-bit chars, 2 stop bits, parity even, and 1200 bps|Because, we will not use interrupts, make sure that the UART is configured not to generate interrupts*/
    lcr = UART_LCR_WLEN8 | UART_LCR_PARITY | UART_LCR_EPAR | UART_LCR_STOP;
    outb(lcr, baseaddr + UART_LCR);
    
    lcr = lcr | UART_LCR_DLAB; // activar o acesso ao registo Divisor Latch
    outb(lcr, baseaddr + UART_LCR);
    
    //Configurar a velocidade (bitrate)
    outb(UART_DIV_1200, baseaddr+UART_DLL);
    outb(0, baseaddr+UART_DLM);

    lcr &= ~UART_LCR_DLAB; // desactivar o acesso ao registo Divisor Latch
    outb(lcr, baseaddr + UART_LCR);

    major = SERP_MAJOR;
    minor = baseMinor;

    /*------ALLOCATING-DEVICE-NUMBERS-----*/
    if (SERP_MAJOR) { //Se estiver definido 
        printk(KERN_INFO "%-10sMajor is defined by user: %d \n","INFO:",SERP_MAJOR);
        dev = MKDEV(SERP_MAJOR,baseMinor);
        result = register_chrdev_region(dev,seriMinors,"seri");
    } else {
        printk(KERN_INFO "%-10sMajor is dynamically set \n","INFO:");
        result = alloc_chrdev_region(&dev, baseMinor, seriMinors,"seri");
        
    }

    if (result < 0) {
        printk(KERN_WARNING "%-10sMajor number allocation failed: %d\n","ERROR:", major);
        return result;
    }         

    major = MAJOR(dev);
    minor = MINOR(dev);

    printk(KERN_INFO "%-10sDevice Number Allocated!! \n","SUCESS:");
    printk(KERN_INFO "%-10sMajor: %d \n", " ",major);
    printk(KERN_INFO "%-10sMinor: %d \n\n", " ",minor);
    /*----------------END-----------------*/



    /*---ALLOCATING-CHAR-DEVICE-STRUCT----*/
    seriDevs = kmalloc(seriMinors * sizeof (struct seri_devices), GFP_KERNEL);

    printk(KERN_INFO "%-10sAllocating %d devices (User defined)\n","INFO:",seriMinors);

    for (i = 0; i < seriMinors; i++) {
        cdev_init(&seriDevs[i].seriCharDev, &seriFops);
        
        seriDevs[i].seriCharDev.ops = &seriFops;
       
        seriDevs[i].seriCharDev.owner = THIS_MODULE;

        result = 0;
        result = cdev_add(&seriDevs[i].seriCharDev, MKDEV(major, i), 1);
        if (result < 0)
            printk(KERN_INFO "%-10sCould not add DEV: %d with minor: \n","ERROR:",MKDEV(major, i), i);
        else
            printk(KERN_INFO "%-10sAdded new DEV: #%d  Minor: %d \n","", MKDEV(major, i), i);
    }
    /*----------------END-----------------*/
      

    /*Configuração da UART - reservar o I/O port range - USAR DISABLESERIAL*/
    request_region(baseaddr, 8, "UART");
    
    //SUCESS!
    printk(KERN_INFO "\n%-10sSERI INITIALIZED!! \n\n","SUCESS:");
    return 0;
}

/**
 *  OPEN
 */
int seri_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Inside seri_open\n");

    struct seri_devices *sdevi;

    sdevi = container_of(inodep->i_cdev, struct seri_devices, seriCharDev);
    filep->private_data = sdevi;



    spin_lock(&(sdevi->lock));


    if (sdevi->count &&
            (sdevi->owner != current->uid) && /* allow user */
            (sdevi->owner != current->euid) && /* allow whoever did su */
            !capable(CAP_DAC_OVERRIDE)) { /* still allow root */
        spin_unlock(&(sdevi->lock));
        return -EBUSY;
    }

    if (sdevi->count == 0) //não havendo ninguém....
        sdevi->owner = current->uid; // passa a estar ocupado pelo requerente

    sdevi->count++;
    spin_unlock(&(sdevi->lock));


    /*Verificar retorno depois - para já serve para informar o kernel de que o device é non seekable*/
    nonseekable_open(inodep, filep);

    kfree(buffer);
    buffer = NULL;

    printk(KERN_ALERT "seri_open() end\n");

    return 0;
}

/*
 * WRITE
 */
ssize_t seri_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp) {

    printk(KERN_INFO "Inside seri write \n");

    int i = 0;
    ssize_t sent = 0;
    unsigned long error;
    unsigned long n;

    /*Alocar espaço em kernel level com tamanho para a mensagem a escrever = count*/
    char *echoKernelBuf;
    echoKernelBuf = kzalloc(count * sizeof (char) + 1, GFP_KERNEL); // tem que ter espaço para o \0

    //ENOMEM = Not enough space
    if (!echoKernelBuf) return -ENOMEM;

    error = copy_from_user(echoKernelBuf, buff, count);


    //Tratamento de erros que podem ocorrer 
    if (error == 0) {
        printk(KERN_INFO "Escreveu tudo = %d \n", (int) error);
        printk(KERN_INFO "%s \n", echoKernelBuf);
    } else
        if (error > 0) {
        printk(KERN_INFO "Falta escrever %lu \n", error);
    }


    for (i = 0; i < strlen(echoKernelBuf) && !(filep->f_flags & O_NONBLOCK) ; i++) //Adicionada flag de O_NONBLOCK
    {

        while (((inb(baseaddr + UART_LSR)) & UART_LSR_THRE) == 0) {
            //schedule(); // se o LSR nao estiver vazio o driver tem de esperar (aqui ele deveria largar o CPU e não fazer busy waiting) e é para isso que serve esta funçao
            //onde aqui está o schedule temos que invocar o IH
            //interrupt_handler(int irq, void* dev_id);

            n = msleep_interruptible(10);

            if (n != 0)
                return -EAGAIN;
        }

        outb(echoKernelBuf[i], (baseaddr + UART_TX));
        sent++; //acumulador do número de caracteres enviados

    }

    kfree(echoKernelBuf);
    printk(KERN_INFO "Seri Write: escreveu %d \n", (int) sent);

    return sent; // retorna o número de caracteres efectivamente escritos;
}

/*
 * READ
 */
ssize_t seri_read(struct file *filep, char __user *buff, size_t count, loff_t *offp) {

    printk(KERN_INFO "\n\n Inside seri read \n");

    int i = 0;
    unsigned long n;
    ssize_t received = 0;
    unsigned long error;

    buffer = (char *) kzalloc(count * sizeof (char) + 1, GFP_KERNEL);

    while (1 && !(filep->f_flags & O_NONBLOCK)) {
        wait_event_interruptible(wq, flag != 0);
        flag = 0;
        //printk(KERN_ALERT " READ : Buffer[%d] = %d | %c \n", increment - 1, buffer[increment - 1], buffer[increment - 1]);
        if (buffer[increment - 1] == '\n' || buffer[increment - 1] == '\r' || buffer[increment - 1] == '\0')
            break;

    }


    copy_to_user(buff, buffer, increment);
    printk(KERN_ALERT "Recebeu %s, com %d chars \n", buff, increment);
    increment = 0;
    
    kfree(buffer);

    return count;

}

/*
 * RELEASE
 */
int seri_release(struct inode *inodep, struct file *filep) {

    struct seri_devices *dev_p = filep->private_data;

    spin_lock(&(dev_p->lock));
    (dev_p->count)--; //liberta o device
    spin_unlock(&(dev_p->lock));

    printk(KERN_INFO "Inside seri release \n");
    return 0;
}

/*
 * EXIT
 */
static void seri_exit(void) {
    int i;

    free_irq(4, seriDevs->dev_irq_id);

    for (i = 0; i < seriMinors; i++) {
        cdev_del(&seriDevs[i].seriCharDev);
        printk(KERN_INFO "Seri Exit: Removido device minor # %d \n", i);
    }

    kfree(seriDevs);

    unregister_chrdev_region(MKDEV(major, 0), seriMinors);
    outb(0x00, baseaddr + UART_LCR); // limpar configurações

    //libertar a região previamente reservada no procedimento de init()
    release_region(baseaddr, 8);

    printk(KERN_INFO " Goodbye:  major number is %d \n", major);


}

module_init(seri_init);
module_exit(seri_exit);
