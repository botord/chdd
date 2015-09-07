/*************************************************************************
	> File Name: chdd.c
	> Author: Dong Hao
	> Mail: haodong.donh@gmail.com
	> Created Time: 2015年08月12日 星期三 16时53分25秒
 ************************************************************************/

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/math64.h>
//#include <asm/system.h>
#include "chdd.h"
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Dong Hao");


#define MEM_SIZE 0x1000 /*4KB*/
#define CHDD_MAJOR 250
#define CHDD_QUANTUM 0x1000 /*4KB*/
#define CHDD_QUANTUM_SET 1000
#define MEM_CLEAR 0x01
#define CHDD_NR_DEVS 1

static int chdd_major = CHDD_MAJOR;
static int chdd_nr_devs = CHDD_NR_DEVS;
static unsigned int chdd_inc = 0;
static u8 chdd_buffer[256];

/* Actually stores the data of the device */
struct chdd_qset {
    /* each of the **data holds quantum * qset memory */
    void **data;
    struct chdd_qset *next;
};

struct chdd {
    struct cdev cdev;
};

/*device structure pointer*/
struct chdd *chddp;

int chdd_release(struct inode *inode, struct file *filp)
{
    chdd_inc--;
    return 0;
}

int chdd_open(struct inode *inode, struct file *filp)
{
    struct chdd *dev;
    if (chdd_inc > 0)
        return -ERESTARTSYS;
    chdd_inc++;

    /*container_of returns the pointer of the upper structure 
    * struct demo_struct { 
    *   type1 member1; 
    *   type2 member2; 
    *   type3 member3; 
    *   type4 member4; 
    * };      
    * 
    * struct demo_struct *demo_p;
    * struct type3 *member3_p = get_member_pointer_from_somewhere();
    * demo_p = container_of(member3_p, struct demo_struct, member3);
    *
    * */
    dev = container_of(inode->i_cdev, struct chdd, cdev);
    filp->private_data = dev;

    /*trim the length of the device if open was write-only 
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        chdd_trim(dev);
    }
    del_timer(&dev->timer);

    if (filp->f_mode & FMODE_READ) {
        dev->nreaders++;
        PDEBUG("Device %d nreaders: %d.\n", DEV_NTH(dev), dev->nreaders);
    }

    if (filp->f_mode & FMODE_WRITE) {
        if (dev->nwriters > 0) {
            up(&dev->sem);
            PDEBUG("Device %d busy writing.\n", DEV_NTH(dev));
            return -EBUSY;
        } else {
        dev->nwriters++;
        PDEBUG("Device %d nwriters: %d.\n", DEV_NTH(dev), dev->nwriters);
        }
    }
    */
//    up(&dev->sem);
    return 0;
}

ssize_t chdd_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    loff_t p = *ppos;
    size_t count = size;
    ssize_t ret = 0;

    if (p >= 256) {
        goto out;
    }

    if (count > 256 - p) {
        count = 256 - p;
    }
    p += count ;


    if (copy_to_user(buf, chdd_buffer + *ppos, count)) {
        ret = -EFAULT;
        goto out;
    } else {
        *ppos += p;
        ret = count;
        printk(KERN_ALERT "read %u bytes(s) from %lld", count, p);
    }
out:
    return ret;
}

ssize_t chdd_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    loff_t p = *ppos;
    size_t count = size;
    ssize_t ret = -ENOMEM;
    
    if (p >= 256) {
        goto out;
    }

    /* write only up to the end of this quantum */
    if (count > 256 - p) {
        count = 256 - p;
    }
    p += count;

    if (copy_from_user(chdd_buffer + *ppos, buf, count)) {
        ret = -EFAULT;
        goto out;
    }
    
    *ppos += p;
    ret = count;

out:
    return ret;
}

/*
int chdd_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
              unsigned long arg)
{
    struct chdd *dev = filp->private_data;

    switch (cmd) {
        case MEM_CLEAR:
            memset(dev->mem, 0, MEM_SIZE);
            printk(KERN_ALERT "chdd memory is set to zero");
            break;

        default:
            return -EINVAL;
    }
    return 0; 
}
*/
loff_t chdd_llseek(struct file *filp, loff_t off, int whence)
{
    loff_t pos = filp->f_pos;
    switch (whence) {
        case 0:
            pos = off;
            break;
        case 1:
            pos += off;
            break;
        case 2:
        default:
            return -EINVAL;
    }

    if (pos < 0 || pos > 256) {
        return -EINVAL;
    }   

    filp->f_pos = pos;

    return pos;
}
const struct file_operations chdd_fops = {
    .owner = THIS_MODULE,
    .read = chdd_read,
    .write = chdd_write,
    .open = chdd_open,
    .release = chdd_release,
    .llseek = chdd_llseek,
};

static void chdd_exit(void)
{
    dev_t devno = MKDEV(chdd_major, 0);

    printk(KERN_INFO "Goodbye cruel world!\n");

    if (chddp) {
        cdev_del(&chddp->cdev);
        kfree(chddp);
    }
    unregister_chrdev_region(devno, chdd_nr_devs);
}
/*
static void chdd_setup_cdev(struct chdd *dev, int index) 
{
    int err, devno = MKDEV(chdd_major, chdd_minor + index);

    cdev_init(&dev->cdev, &chdd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &chdd_fops;// I don't know whether I need it or not.
    err = cdev_add(&dev->cdev, devno, 1);

    if (err) {
        printk(KERN_ALERT "Error %d adding chdd %d\n", err, index);
        printk(KERN_ALERT "Failed in setup_cdev!\n");
 //       chdd_exit();
    }
}
*/
static int chdd_init(void)
{
    int result;
    dev_t devno = MKDEV(chdd_major, 0);

    /*apply for the device number*/
    if (chdd_major) {
        result = register_chrdev_region(devno, chdd_nr_devs, "chdd");
    }

    if (result < 0) {
        printk(KERN_ALERT "chdd cannot get major %d\n, minor count %d"
                            , chdd_major, chdd_nr_devs);
        return result;
    }
    printk(KERN_ALERT "chdd get major %d\n", chdd_major);

    /*apply memory for device struct*/
    chddp = kmalloc(chdd_nr_devs * sizeof(struct chdd), GFP_KERNEL);
    if (!chddp) {
        PDEBUG("Allocate device error!");
        result = -ENOMEM;
        goto fail;
    } 

    memset(chddp, 0, chdd_nr_devs * sizeof(struct chdd));

    cdev_init(&(chddp->cdev), &chdd_fops);
    chddp->cdev.owner = THIS_MODULE;
    chddp->cdev.ops = &chdd_fops;// I don't know whether I need it or not.
    result = cdev_add(&(chddp->cdev), devno, 1);

    if (result) {
        printk(KERN_ALERT "Error %d adding chdd %d\n", result, 0);
        printk(KERN_ALERT "Failed in setup_cdev!\n");
        PDEBUG("Initialized device %d, size: %d.", i+1, chddp[i].size);
        goto fail;
    }
    
    printk(KERN_ALERT "Hello, world!\n");
    return 0;

fail:
    chdd_exit();
    //unregister_chrdev_region(devno, 1);
    return result;
}

module_init(chdd_init);
module_exit(chdd_exit);
