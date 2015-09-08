#!/bin/bash

make
rmmod test
rm /dev/chdd

insmod test.ko 
mknod /dev/chdd c 250 0
