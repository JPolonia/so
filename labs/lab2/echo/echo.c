/*                                                     
 * $Id: hello.c,v 1.5 2004/10/26 03:32:21 corbet Exp $ 
 */                                                    
#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("Dual BSD/GPL");

static int hello_init(void)
{
	printk(KERN_ALERT "Hello, world Easy!\n");
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_ALERT "Goodbye, cruel world ez\n");
}

module_init(hello_init);
module_exit(hello_exit);

/*
	Allocate Memory
	void *kmalloc( size_t size, gfp_t flags); GFP_KERNEL
	void *kzalloc( size_t size, gfp_t flags);
	void kfree(void *obj);

	Install/Removal Kernel Module
	sudo insmod hello.ko 
	sudo rmmod hello
*/
