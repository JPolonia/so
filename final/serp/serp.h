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
extern int serp_major;     /* serp.c */
extern int serp_devs;

struct serp_devices{
  struct cdev serpCharDev; //Estrutura no kernel que representa o char device 
};

