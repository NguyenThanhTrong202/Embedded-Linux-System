#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "asgn1_ioctl.h"

int main(void)
{
    int fd;
    struct asgn1_stats stats;

    fd = open("/dev/asgn1", O_RDONLY);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, ASGN1_GET_STATS, &stats) < 0) {
        perror("GET_STATS");
        close(fd);
        return 1;
    }

    printf("===== DRIVER STATISTICS =====\n");
    printf("Open count      : %d\n", stats.open_count);
    printf("Write count     : %d\n", stats.write_count);
    printf("Read count      : %d\n", stats.read_count);
    printf("Buffer length   : %d\n", stats.buffer_len);
    printf("Last write size : %d\n", stats.last_write_size);

    close(fd);

    return 0;
}

