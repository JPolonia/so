/*
 * Macros to help debugging
 */

#undef PDEBUG             /* undef it, just in case */
#ifdef SCULLC_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "scullc: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

/*
 * serp specific stuff
 */

#define SERP_MAJOR 0   /* dynamic major by default */
#define SERP_DEVS 4    /* serp0 through serp3 */


/*
 * The different configurable parameters
 */
extern int seri_major;     /* serp.c */
extern int seri_devs;


/*
 *in the case of an interrupt-driven DD you need buffers for communication between the IH and the user threads and wait queues for these threads to wait for data availability. 
 * Furthermore, you will need at least one semaphore to ensure that if multiple threads attempt to execute concurrent system calls to access the same device no race conditions occur. 
 * (Actually, the use of a semaphore is also needed in the case of polled operation.) 
 * It is therefore convenient to group all these variables, including the struct cdev, in a single data structure, such as seri_dev_t outlined in pg. 29
 * 
 * Ou seja: são precisos buffers para comunicar entre o IH e as threads do user; e também filas de espera que ficam a aguardar que a info fique disponível
 */

//struct seriBuf{
//  char *buf;
//  spinlock_t lock; // lock para evitar problemas de concorrencia no acesso ao buffer;
//  // int counter;  // contador onde é colocada a informação relativa ao número de caracteres armazenados
//  // int in;
//  // int out;
//};



/** Os elementos introduzidos estão de acordo com o indicado na pág. 29 dos slides sobre Kernel Concurrency **/

struct seri_devices{
  struct cdev seriCharDev; //Estrutura no kernel que representa o char device
  void *dev_irq_id; // é o que a gente quiser,....
  spinlock_t lock; // para evitar problemas de concorrência no acesso a parametros da estrutura
  int num_users;
  int owner; //numero do utilizador actual
  int count; // uma flag que vai indicar se o device driver está em utilização ou não;
  char *buffer; //criado para suprimir a utilização do global;
  int minor; //criado para suprimir o global
  int major; //criado para suprimir o global
};

