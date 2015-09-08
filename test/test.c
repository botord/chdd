/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007, 2010 fengGuojin(fgjnew@163.com)
 */
#include<sys/types.h>
#include<unistd.h>
#include<fcntl.h>
#include<linux/rtc.h>
#include<linux/ioctl.h>
#include<stdio.h>
#include<stdlib.h>

main()
{
    char data[256];
    int result;
    int fd = open("/dev/chdd", O_RDWR);

    if (fd == -1) {
        perror("error open\n");
        exit(-1);
    }

    printf("open /dev/chdd successfully\n");
  
    result = write(fd, "fgj", 3);
    
    if (result == -1) {
        perror("write error\n");
        exit(-1);
    }
    
    result = lseek(fd, 0, 0);
    if (result == -1) {
        perror("lseek error\n");
        exit(-1);
    } 
    result = read(fd, data, 3);

    if(result ==-1) {
        perror("read error\n");
        exit(-1);
    }
    printf("read successfully:%s\n",data);
    close(fd);
}
