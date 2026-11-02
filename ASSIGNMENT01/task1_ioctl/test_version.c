#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "asgn1_ioctl.h"

int main(void)
{
    int fd;
    char version[ASGN1_VERSION_SIZE] = {0};

    /* Mo device */
    fd = open("/dev/asgn1", O_RDONLY);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Goi ioctl GET_VERSION */
    if (ioctl(fd, ASGN1_GET_VERSION, version) < 0) {
        perror("GET_VERSION");
        close(fd);
        return 1;
    }

    /* Hien thi ket qua */
    printf("Driver version: %s\n", version);

    close(fd);

    return 0;
}
