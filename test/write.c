#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
int main(int argc, char *argv[])
{
	int fd;
	char num[3] = {'\0'};
	int i=0;

	fd = open("/dev/chdd", O_RDWR, S_IRUSR | S_IWUSR);
	if (fd != - 1) {
        if (argc > 2) {
            num[0] = 0x35;
            write(fd, num, 2);
		    printf("num = %s\n", num);
            close(fd);
            return 0;
        } 
        num[0]='F';
        num[1]='G';
        write(fd, num, 2);
	    close(fd);
	} else {
		printf("device open failure\n");
	}

    return 0;
}
