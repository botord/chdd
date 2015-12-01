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
#include <asm/atomic.h>
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
#define CHDD_QUANTUM 0x1000 /*4KB*/
#define CHDD_QUANTUM_SET 1000
#define MEM_CLEAR 0x01

static int chdd_major = CHDD_MAJOR;
static int chdd_minor = CHDD_MINOR;
static int chdd_nr_devs = CHDD_NR_DEVS;
static int flag = 0;

/* Actually stores the data of the device */
struct chdd_qset {
    /* each of the **data holds quantum * qset memory */
    void **data;
    struct chdd_qset *next;
};


struct chdd {
    struct cdev cdev;
    unsigned char chdd_buffer[256];
    unsigned int size;
    struct semaphore sem;
    wait_queue_head_t wq;
    //atomic_t chdd_inc;
    int chdd_inc;
};

/*device structure pointer*/
struct chdd *chddp;

int chdd_release(struct inode *inode, struct file *filp)
{
    struct chdd *dev = filp->private_data;

    dev->chdd_inc++;
    PDEBUG("chdd_release: device closed! chdd_inc = %d\n", dev->chdd_inc);
    up(&dev->sem);
    return 0;
}

int chdd_open(struct inode *inode, struct file *filp)
{
    struct chdd *dev;

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

    /*
     * if (down_interruptible(&dev->sem)) {
        PDEBUG("unable to open this device!\n");
        return -ERESTARTSYS;
    }
    */
    if (!(dev->chdd_inc > 0)) {
        PDEBUG("device busy! chdd_inc = %d\n", dev->chdd_inc);
        return -EBUSY;
    } 
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
    PDEBUG("device opened!\n");
    dev->chdd_inc--;
    //up(&dev->sem);
    return 0;
}

ssize_t chdd_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    loff_t p = *ppos;
    size_t count = size;
    ssize_t ret = 0;
    struct chdd *dev = filp->private_data;

    if (p >= 256) {
        goto out;
    }

    if (count > 256 - p) {
        count = 256 - p;
    }

    if (wait_event_interruptible(dev->wq, flag) != 0) {
        return -ERESTARTSYS;
    }

    flag = 0;

    if (down_interruptible(&dev->sem)) {
        return -ERESTARTSYS;
    }

    //if (copy_to_user(buf, chdd_buffer + *ppos, count)) {
    if (copy_to_user(buf, dev->chdd_buffer + p, count)) {
        ret = -EFAULT;
        goto out;
    } else {
        *ppos += p;
        ret = count;
        printk(KERN_ALERT "read %u bytes(s) from %lld\n", count, p);
    }

out:
    up(&dev->sem);
    return ret;
}

ssize_t chdd_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    loff_t p = *ppos;
    struct chdd *dev = filp->private_data;
    size_t count = size;
    ssize_t ret = -ENOMEM;

    if (p >= 256) {
        goto out;
    }

    /* write only up to the end of this quantum */
    if (count > 256 - p) {
        count = 256 - p;
    }
    if (down_interruptible(&dev->sem)) {
        return -ERESTARTSYS;
    }

    if (copy_from_user(dev->chdd_buffer + p, buf, count))
    {
        ret = -EFAULT;
        goto out;
    }

    *ppos += p;
    ret = count;
    flag = 1;
    up(&dev->sem);
    wake_up_interruptible(&dev->wq);
    return ret;

out:
    up(&dev->sem);
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
    switch (whence)
    {
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

    if (pos < 0 || pos > 256)
    {
        return -EINVAL;
    }

    filp->f_pos = pos;

    return pos;
}
const struct file_operations chdd_fops =
{
    .owner = THIS_MODULE,
    .read = chdd_read,
    .write = chdd_write,
    .open = chdd_open,
    .release = chdd_release,
    .llseek = chdd_llseek,
};

static void chdd_exit(void)
{
    int index;
    dev_t devno = MKDEV(chdd_major, 0);

    if (chddp) {
        for (index = 0; index < chdd_nr_devs; index++) {
            cdev_del(&chddp[index].cdev);
            printk(KERN_INFO "CHDD: chdd_nr_devs = %d\n", index);
        }
        kfree(chddp);
    }
    unregister_chrdev_region(devno, chdd_nr_devs);
    printk(KERN_INFO "Goodbye cruel world!\n");
}

/* 
 * Return:
 *  - 0 if Succeeded
 *  - 1 if Failed
 *  */
static int chdd_setup_cdev(struct chdd *dev, int index)
{
    int err, devno = MKDEV(chdd_major, chdd_minor + index);

    cdev_init(&(dev->cdev), &chdd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &chdd_fops;// I don't know whether I need it or not.
    err = cdev_add(&dev->cdev, devno, 1);

    if (err) {
        printk(KERN_ALERT "Error %d adding chdd %d\n", err, index);
        printk(KERN_ALERT "Failed in setup_cdev!\n");
    }
    PDEBUG("Initialized device %d, size: %d.\n", index+1, chddp[index].size);
    return err;
}

static int chdd_init(void)
{
    int result;
    int index;
    dev_t devno = MKDEV(chdd_major, 0);

    /*apply for the device number*/
    if (chdd_major) {
        printk(KERN_INFO "CHDD_MAJOR is %d", chdd_major);
        result = register_chrdev_region(devno, chdd_nr_devs, "chdd");
    }
    else {
        result = alloc_chrdev_region(&devno, 0, chdd_nr_devs, "chdd");
        /* Allocate device number if no chdd_major. */
        chdd_major = MAJOR(devno);
    }

    if (result < 0) {
        printk(KERN_ALERT "chdd cannot get major %d\n, minor count %d"
               , chdd_major, chdd_nr_devs);
        return result;
    }
    PDEBUG("chdd get major %d\n", chdd_major);

    /*apply memory for device struct*/
    chddp = kmalloc(chdd_nr_devs * sizeof(struct chdd), GFP_KERNEL);
    if (!chddp) {
        PDEBUG("Allocate device error!");
        result = -ENOMEM;
        goto fail;
    }

    memset(chddp, 0, chdd_nr_devs * sizeof(struct chdd));

    PDEBUG("Hello World!\n");
    for (index = 0; index < chdd_nr_devs; index++) {
        result = chdd_setup_cdev(&chddp[index], index);
        sema_init(&chddp[index].sem, 1);
        chddp[index].chdd_inc = 1;
        init_waitqueue_head(&chddp[index].wq);
        if (result) {
            goto fail;
        }
    }
    PDEBUG("chdd_nr_devs = %d, devices initialized\n", index);

    return 0;

fail:
    chdd_exit();
    return result;
}

module_init(chdd_init);
module_exit(chdd_exit);
