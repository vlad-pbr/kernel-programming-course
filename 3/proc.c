// proc.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define DEVICE_NAME "km"
#define BUFFER_LEN 4096

// ioctl commands
#define GET_PHYS_MEM _IOWR(234, 100, char*)
#define GET_CR3 _IOR(234, 101, unsigned long*)

int main(int argc, char *argv[]) {

    // define variables
    char *device_path;
    int device_fd;
    int64_t cr3;
    char buffer[BUFFER_LEN];
    int32_t segment;

    // make device path
    device_path = malloc(6 + strlen(DEVICE_NAME));
    sprintf(device_path, "/dev/%s", DEVICE_NAME);

    // open device
    device_fd = open(device_path, O_RDWR);

    // handle physical memory reading
    if (strcmp(argv[1], "100") == 0) {
        
        // parse segment number
        segment = atoi(argv[2]);

        // copy segment number as bytes to buffer
        memcpy(buffer, (char*)&segment, sizeof(segment));

        // request physical memory from module
        ioctl(device_fd, GET_PHYS_MEM, (char*)&buffer);

        // TODO display and compare
    }

    // handle CR3 register
    else if (strcmp(argv[1], "101") == 0) {
        ioctl(device_fd, GET_CR3, (unsigned long*)&cr3);
        printf("%ld\n", cr3);
    }

    // close device
    close(device_fd);

}