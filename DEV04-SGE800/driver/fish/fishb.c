#include "fishb.h"

MODULE_LICENSE("GPL");
/******************************************
	嵌入式系统使用固定设备号
******************************************/
static dev_t dev_fishb = MKDEV(222,0);

struct cdev cdev_fishb;
struct class *fishb_class;



int fishb_open(struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t fishb_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int retval = -1;
	void *p;
	unsigned int addr;
	
	printk(KERN_ALERT "read!\n");	
	if (copy_from_user(&addr, buf, 4)) {
			retval = -EFAULT;
			goto out;
	}	
	printk("addr = %x\n",addr);	
	p = ioremap (addr, 4);
	if (copy_to_user(buf, p, 4)) {
			retval = -EFAULT;
			goto out;
	}
	retval = (unsigned long) p;
	iounmap(p);
out:
	return retval;
}

ssize_t fishb_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int retval = -1;
	unsigned int *p;
	unsigned int addr;
	//unsigned int reg = (unsigned int)count;
	
	printk(KERN_ALERT "write! \n");	
	//addr =(unsigned int) count;
	//传地址
	if (copy_from_user(&addr, buf, 4)) {
			retval = -EFAULT;
			goto out;
	}
	printk(KERN_ALERT "addr = %x \n",addr); 

	p = ioremap (addr, 4);	
	*p = count;
	//把写的寄存器返回
	if (copy_to_user(buf, p, 4)) {
		retval = -EFAULT;
		goto out;
	}
	retval = (unsigned long) p;
	iounmap(p);
out:
	return retval;
}

int fishb_ioctl(struct inode *inode, struct file *filp, unsigned int arg1, unsigned long arg2)
{
	return 0;
}

int fishb_release(struct inode *inode, struct file *filp)
{
	return 0;
}

loff_t fishb_llseek(struct file *filp, loff_t offset, int arg)
{
	//printk("f_pos = %x\n",offset);	
	switch(arg) {
		case 0:
			filp->f_pos = offset;
			break;
		case 1:
			filp->f_pos += offset;
			break;
		case 2:
			filp->f_pos = offset + 0xff;
			break;
		default:
			filp->f_pos = 0;
			break;
	}
	return 0;	
}




struct file_operations fishb_fops = {
	.owner = THIS_MODULE,
	.llseek = fishb_llseek,
	.read = fishb_read,
	.write = fishb_write,
	.ioctl = fishb_ioctl,
	.open = fishb_open,
	.release = fishb_release,
};

/******************************************
           called when insmod
******************************************/
static int __init fishb_init(void)
{
	int retval;
	retval = register_chrdev_region (dev_fishb, 1, "fishb");
	if (retval)
		return retval;
	printk(KERN_ALERT "Major: %d;Minor: %d\n", MAJOR(dev_fishb),MINOR(dev_fishb));
	cdev_init(&cdev_fishb, &fishb_fops);
	cdev_fishb.owner = THIS_MODULE;
	retval = cdev_add(&cdev_fishb, dev_fishb, 1);
	if (retval)
		return retval;
	printk(KERN_ALERT "char dev register successful!\n");
	/* creating   class */
    fishb_class = class_create(THIS_MODULE, "fishb");
    if(IS_ERR(fishb_class)) {
        printk("Err: failed in creating class.\n");
        return -1;
    }	
    /* register your own device in sysfs, and this will cause udevd to create corresponding device node */
    device_create(fishb_class, NULL, dev_fishb, NULL, "fishb");
	return 0;
}
/******************************************
           called when rmmod
******************************************/
static void __exit fishb_exit(void)
{
	device_destroy(fishb_class, dev_fishb);
	class_destroy(fishb_class);	
	cdev_del(&cdev_fishb);
	unregister_chrdev_region (dev_fishb, 1);
	
	printk(KERN_ALERT "Goodbye!\n");
}

module_init(fishb_init);
module_exit(fishb_exit);
