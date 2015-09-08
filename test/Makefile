CC = gcc

read-objs := read.o
write-objs := write.o

all: read write

read:
	$(CC) -O1 read.c -o read
write:
	$(CC) -O1 write.c -o write 

clean:
	rm -rf read write
