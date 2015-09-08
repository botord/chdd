KVERS = $(shell uname -r)
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
