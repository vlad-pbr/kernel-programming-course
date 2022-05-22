// proc.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_NAME "km"

// ioctl commands
#define GET_CR3 _IOR(234, 101, unsigned long*)

int main(int argc, char *argv[]) {

    // define variables
    int device_fd;
    int64_t cr3;

    // open device
    device_fd = open("/dev/km", O_RDWR);

    // handle CR3 register
    if (strcmp(argv[1], "101") == 0) {
        ioctl(device_fd, GET_CR3, (unsigned long*)&cr3);
        printf("%ld\n", cr3);
    }

    // close device
    close(device_fd);

}