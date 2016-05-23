#ifndef _ECHO_H_
#define _ECHO_H_


#ifndef ECHO_MAJOR
#define ECHO_MAJOR 0
#endif

#include <linux/slab.h>
#include <linux/ioctl.h>

int echo_init(void);

void echo_exit(void);

extern int echo_major;
extern int echo_minor;

struct echo_dev{
    
    struct cdev cdev;

};

extern struct file_operations echo_fops;

int echo_open(struct inode *inode, struct file *filp);
int echo_release(struct inode *inode, struct file *filp);
static void echo_create_cdev(struct echo_dev *dev);

ssize_t echo_read(struct file *filep, char __user *buff, size_t count, loff_t *offp);
ssize_t echo_write(struct file *filep, const char __user *buff, size_t count, loff_t *offp);
extern int nonseekable_open(struct inode * inode, struct file * filp);

unsigned long copy_to_user(void __user *to,const void *from,unsigned long count);
unsigned long copy_from_user(void *to, const void __user *from, unsigned long count);


#endif //Echo.h
