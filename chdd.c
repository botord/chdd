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
//#include <linux/types.h>
//#include <linux/sched.h>
#include <linux/mm.h>
//#include <asm/io.h>
//#include <asm/system.h>


#define MEM_SIZE 0x1000 /*4KB*/
#define CHDD_MAJOR 250
#define CHDD_QUANTUM 0x1000 /*4KB*/
#define CHDD_QUANTUM_SET 1000
#define MEM_CLEAR 0x01

static int chdd_quantum = CHDD_QUANTUM;
static int chdd_quantum_set = CHDD_QUANTUM_SET;
static int chdd_major = CHDD_MAJOR;
static int chdd_minor = 0;

/* Actually stores the data of the device */
struct chdd_qset {
    /* each of the **data holds quantum * qset memory */
    void **data;
    struct chdd_qset *next;
};

struct chdd {
    struct chdd_qset *data;     /* Pointer to first quantum set */
    int quantum;                /* the current quantum size */
    int qset;                   /* the current array size */
    unsigned long size;         /* amount of data stored here */
    struct semaphore sem;       /* mutual exclusion semaphore */
    struct cdev cdev;
    unsigned char mem[MEM_SIZE];
};

/*device structure pointer*/
struct chdd *chddp;

int chdd_release(struct inode *inode, struct file *filp)
{
    struct chddp *chddp = filp->private_data;
    kfree(chddp);
    return 0;
}

int chdd_trim(struct chdd *dev) {
    struct chdd_qset *next, *dptr;
    int qset = dev->qset;
    int i;

    for (dptr = dev->data; dptr; dptr = next) {
        if (dptr->data) {
            for (i = 0; i < qset; i++) {
                kfree(dptr->data[i]);/*clear one quantum each time */
                dptr->data = NULL;
            }
            /* quantum * qset */
        }
        next = dptr->next;
        kfree(dptr);
    }
    dev->size = 0;
    dev->quantum = chdd_quantum; 
    dev->qset = chdd_quantum_set;
    dev->data = NULL;

    return 0;
}

int chdd_open(struct inode *inode, struct file *filp)
{
    /*filp->private_data = dev;*/
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

    /*trim the length of the device if open was write-only */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        chdd_trim(dev);
    }
    return 0;
}

ssize_t chdd_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;
    ssize_t ret = 0;
    struct chdd *dev = filp->private_data;
    /*we let this parameter points to chdd structure when we do chdd_open*/

    /**/
    if (p > MEM_SIZE) {
        return 0;
    }

    if (count > MEM_SIZE - size) {
        count = MEM_SIZE - size;
    }

    if (copy_to_user(buf, dev->mem + p, count)) {
        ret = -EFAULT;
    } else {
        *ppos += count;
        ret = count;
        printk(KERN_INFO "read %u bytes(s) from %lu", count, p);
    }
    return ret;
}
ssize_t chdd_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    unsigned long p = *ppos;
    unsigned int count = size;
    ssize_t ret = 0;

    struct chdd *dev = filp->private_data;

    if (p > MEM_SIZE) {
        return ret;
    }
    if (count > MEM_SIZE - p) {
        count = MEM_SIZE - p;
    }

    if (copy_from_user(dev->mem + p, buf, count)) {
        ret = count;
        printk(KERN_INFO "write %u bytes(s) from %lu", count, p);
    }

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
            printk(KERN_INFO "chdd memory is set to zero");
            break;

        default:
            return -EINVAL;
    }
    return 0; 
}
*/
loff_t chdd_llseek(struct chdd *dev, loff_t *ppos, int whence)
{
    loff_t ret = 0;
    return ret;
}
const struct file_operations chdd_fops = {
    .owner = THIS_MODULE,
    .read = chdd_read,
    .write = chdd_write,
//    .ioctl = chdd_ioctl,
    .open = chdd_open,
    .release = chdd_release,
//    .llseek = chdd_llseek,
};

static void __exit chdd_exit(void)
{
    dev_t devno = MKDEV(chdd_major, 0);

    printk(KERN_INFO "Goodbye cruel world!");

    if (chddp) {
            cdev_del(&chddp[1].cdev);
            cdev_del(&chddp[2].cdev);
            kfree(&chddp[1].cdev);
            kfree(&chddp[2].cdev);
    }
    kfree(chddp);
    unregister_chrdev_region(devno, 2);
}

int chdd_setup_cdev(struct chdd *dev, int index) 
{
    int err, devno = MKDEV(chdd_major, chdd_minor + index);

    cdev_init(&dev->cdev, &chdd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &chdd_fops;
    err = cdev_add(&dev->cdev, devno, 1);

    if (err) {
        printk(KERN_NOTICE "Error %d adding chdd %d", err, index);
        chdd_exit();
    }

    return 0;
}

static int __init chdd_init(void)
{
    int result;
    dev_t devno = MKDEV(chdd_major, 0);

    /*apply for the device number*/
    if (chdd_major) {
        result = register_chrdev_region(devno, 2, "chdd");
    } else {
        result = alloc_chrdev_region(&devno, 0, 2, "chdd");
    }

    if (result < 0) {
        printk(KERN_WARNING "chdd cannot get major %d", chdd_major);
        return result;
    }

    /*apply memory for device struct*/
    chddp = kmalloc(2*sizeof(struct chdd), GFP_KERNEL);
    if (!chddp) {
        result = -ENOMEM;
        goto fail;
    } 

    memset(chddp, 0, 2*(sizeof(struct chdd)));

    result = chdd_setup_cdev(&chddp[0], 0);
    if (!result) {
        goto fail_2;
    }

    result = chdd_setup_cdev(&chddp[1], 1);
    if (!result ) {
        goto fail_1;
    }
    return 0;

fail:
fail_1:
    unregister_chrdev_region(devno, 2);
fail_2:
    return result;
}

module_init(chdd_init);
module_exit(chdd_exit);
