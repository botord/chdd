KVERS = $(shell uname -r)


# comment/uncomment the following line to disable/enable debugging
DEBUG = y

ifeq ($(DEBUG),y)
	DEBFLAGS = -O -g -DCHDD_PDEBUG # "-O" is needed to expand inlines
else
	DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)
CC = gcc
obj-m += test.o
test-objs := chdd.o

build: kernel_modules

install:
	insmod test.ko ; mknod /dev/chdd c 250 0

uninstall:
	rmmod test; rm /dev/chdd

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) 
clean:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean 
