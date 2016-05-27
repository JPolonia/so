#include <linux/types.h> //dev_t,MAJOR(),MINOR(),MKDEV(),...
#include <linux/fs.h> //struct file_operations, struct file
#include <linux/cdev.h> //struct cdev,cdev_alloc(),...

MODULE_LICENSE("Dual BSD/GPL");

//CDEV STRUCTURE
struct serp_dev {
	//struct scull_qset *data;  /* Pointer to first quantum set */
    //int quantum;              /* the current quantum size */
    //int qset;                 /* the current array size */
    //unsigned long size;       /* amount of data stored here */
    //unsigned int access_key;  /* used by sculluid and scullpriv */
    //struct semaphore sem;     /* mutual exclusion semaphore     */
    struct cdev *cdev;     /* Char device structure      */
    struct kfifo *rxfifo;
	dev_t dev;  
	char c;
	char *buff;
	int flag;
	int flagr;
	int sent;
	int size;
}

struct file_operations serp_fops = {
		.owner 	 =	THIS_MODULE,
		.llseek  =	serp_llseek,
		.read 	 =	serp_read,
		.write 	 =	serp_write,
		//.ioctl 	 =	serp_ioctl,
		.open 	 =	serp_open,
		.release = 	serp_release,
};

serp_dev *device; 


static int serp_init(void){
	int serp_major = 0; //CHANGE FOR MANUAL MAJOR ASSIGN
	int serp_minor = 0;
	int result = -1;
	int serp_nr_devices = 3;
	dev_t dev;

	/*------ALLOCATING-DEVICE-NUMBERS-----*/
	if (serp_major)	{
		dev = MKDEV(serp_major,serp_minor);
		result = register_chrdev_region(dev,serp_nr_devices,"seri");
	} else {
		result = alloc_chrdev_region(&dev, serp_minor, serp_nr_devs,"seri");
		serp_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "seri: can't get major %d\n", serp_major);
		return result;
	}
	/*----------------END-----------------*/

	/*---ALLOCATING-CHAR-DEVICE-STRUCT----*/
	device = kmalloc(sizeof(serp_dev) * serp_minor,GFP_KERNEL);
	for(i = 0; i < serp_minor; i++){
		device[i]->cdev = cdev_alloc();
		(device->cdev)->owner = THIS_MODULE;
		(device->cdev)->ops = &fops;

	}

	/*----------------END-----------------*/

	/*------CHAR-DEVICE-REGISTRATION------*/
	/*----------------END-----------------*/

	return 0 //sucess
}

module_init(serp_init);
module_exit(serp_exit);