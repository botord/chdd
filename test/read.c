#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
int main()
{
	int fd;
	char num[2];

	fd = open("/dev/chdd", O_RDWR, S_IRUSR | S_IWUSR);
	if (fd != - 1) {
        while (1) {
            read(fd, &num,2);
            printf("The data is %s\n", num);
            //exit if input equals 5
            if (num[0] == 0x35) {
                printf("Get 0x35, will exit.\n");
                close(fd);
                break;
            }
        }
	} else {
        printf("device open failure\n");
	}

    return 0;
}
