/*
 * =====================================================================================
 *
 *       Filename:  chdd.h
 *
 *    Description:  character device driver header file.
 *
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  gcc
 *       Modifier:  Dong Hao
 *
 * =====================================================================================
 */

#ifndef  CHDD_INC
#define  CHDD_INC

//#include	<linux/semaphore.h> 

#undef PDEBUG                                   /*  undef it, just in case */
#ifdef CHDD_DEBUG
#  ifdef __KERNEL__
     /*  This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "chdd: " fmt, ## args)
#  else
     /*  This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...)                  /*  not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...)                   /*  nothing: it's a placeholder */

#ifndef CHDD_MAJOR
#define CHDD_MAJOR 0                            /*  dynamic major by default */
#endif

#ifndef CHDD_NR_DEVS
#define CHDD_NR_DEVS 1                          /*  CHDD0 through CHDD2 */
#endif

#ifndef  BLOCK_SIZE
#define BLOCK_SIZE 1024                         /* minimize device block */
#endif

#undef DEV_NTH
#define DEV_NTH(dev) dev->size / BLOCK_SIZE     /* nth device */

#ifndef  Z_TIMEOUT
#define Z_TIMEOUT 10                            /* z_timer timeout */
#endif

#define CHDD_IOC_MAGIC 'k' 
#define CHDD_IOCSHOW _IO(CHDD_IOC_MAGIC, 0)    /* show the effective data size */
#define CHDD_IOC_MAXNR 0                        /* max number of ioctl command */


#endif  
