/*************************************************************************
	> File Name: chdd.c
	> Author: Dong Hao
	> Mail: 
	> Created Time: 2015年08月12日 星期三 16时53分25秒
 ************************************************************************/

#include <linux/init.h>
#include <linux/slab.h>
//#include <linux/types.h>
#include <linux/fs.h>
#include <linux/module.h>
//#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/errno.h>
//#include <linux/mm.h>
//#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>


#define MEM_SIZE 0x1000 /*4KB*/
#define CHDD_MAJOR 250
#define MEM_CLEAR 0x01

static int chdd_major = CHDD_MAJOR;
struct chdd {
    struct cdev cdev;
    unsigned char mem[MEM_SIZE];
};

/*device structure pointer*/
struct chdd *chddp;

int chdd_release(void)
{

    return 0;
}

int chdd_open(struct inode *inode, struct file *filp)
{
    filp->private_data = chddp;
    return 0;
}
int chdd_close(void)
{
    return 0;
}

int chdd_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;
    int ret = 0;
    struct chdd *dev = filp->private_data;
    /*we let this parameter points to chdd structure when we do chdd_open*/

    /**/
    if (p > MEM_SIZE) {
        return 0;
    }
    if (count > MEM_SIZE - size) {
        count = MEM_SIZE - size;
    }

    if (copy_to_user(buf, dev->mem + p), count) {
        ret = -EFAULT;
    } else {
        *ppos += count;
        ret = count;
        printk(KERN_INFO "read %u bytes(s) from %lu", count, p);
    }
    return ret;
}

int chdd_write(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;
    int ret = 0;

    struct chdd *dev = filp->private_data;

    if (p > MEM_SIZE) {
        return 0;
    }
    if (count > MEM_SIZE - p) {
        count = MEM_SIZE - p;
    }

}

int chdd_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
              unsigned long arg)
{
    struct chdd *dev = filp->private_data;

    switch (cmd) {
        case MEM_CLEAR:
            memset(dev->mem, 0, MEM_SIZE);
            printk(KERN_INFO "chdd memory is set to zero");
            break;

        default:
            return -EINVAL;
    }
    return 0; 
}

int chdd_llseek(void)
{
    return 0;
}
static const struct file_operations chdd_fops = {
    .owner = THIS_MODULE,
    .read = chdd_read,
    .write = chdd_write,
    .ioctl = chdd_ioctl,
    .open = chdd_open,
    .close = chdd_close,
    .release = chdd_release,
    .llseek = chdd_llseek,
};

static void chdd_set_up_cdev(struct chdd *dev, int index) 
{
    int err, devno = MKDEV(chdd_major, index);

    cdev_init(&dev->cdev, &chdd_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);

    if (err) 
        printk(KERN_NOTICE "Error %d adding chdd %d", err, index);
}

int chdd_init(void)
{
    int result;
    dev_t devno = MKDEV(chdd_major, 0);

    /*apply for the device number*/
    if (chdd_major) {
        result = register_chrdev_region(devno, 1, "chdd");
    } else {
        result = alloc_chrdev_region(&devno, 0, 1, "chdd");
    }

    if (result < 0) {
        return result;
    }

    /*apply memory for device struct*/
    chddp = kmalloc(sizeof(struct chdd), GFP_KERNEL);
    if (!chddp) {
        result = -ENOMEM;
        goto end;
    } 

    memset(chddp, 0, sizeof(sizeof(struct chdd)));
    chdd_set_up_cdev(chddp, 0);
    return 0;

end:
    unregister_chrdev_region(devno, 1);
    return result;
}

static void chdd_exit(void)
{
    printk(KERN_INFO "Goodbye cruel world!");
    cdev_del(&chddp->cdev);
    kfree(chddp);
    unregister_chrdev_region(MKDEV(chdd_major, 0), 1);
}

module_init(chdd_init);
module_exit(chdd_exit);