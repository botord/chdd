/*
 * =====================================================================================
 *
 *       Filename:  chdd.h
 *
 *    Description:  Linux Device Driver exp2.
 *
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#ifndef  DEMO_INC
#define  DEMO_INC

//#include	<linux/semaphore.h> 

#undef PDEBUG                                   /*  undef it, just in case */
#ifdef DEMO_DEBUG
#  ifdef __KERNEL__
     /*  This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "demo: " fmt, ## args)
#  else
     /*  This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...)                  /*  not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...)                   /*  nothing: it's a placeholder */

#ifndef DEMO_MAJOR
#define DEMO_MAJOR 0                            /*  dynamic major by default */
#endif

#ifndef DEMO_NR_DEVS
#define DEMO_NR_DEVS 3                          /*  demo0 through demo2 */
#endif

#ifndef  BLOCK_SIZE
#define BLOCK_SIZE 1024                         /* minimize device block */
#endif

#undef DEV_NTH
#define DEV_NTH(dev) dev->size / BLOCK_SIZE     /* nth device */

#ifndef  Z_TIMEOUT
#define Z_TIMEOUT 10                            /* z_timer timeout */
#endif

#define DEMO_IOC_MAGIC 'k' 
#define DEMO_IOCSHOW _IO(DEMO_IOC_MAGIC, 0)    /* show the effective data size */
#define DEMO_IOC_MAXNR 0                        /* max number of ioctl command */


#endif  
