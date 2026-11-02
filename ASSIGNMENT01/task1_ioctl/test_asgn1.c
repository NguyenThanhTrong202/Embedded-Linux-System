#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "asgn1_ioctl.h"

#define CHECK(cond, name)                         \
    do {                                          \
        if (!(cond)) {                            \
            printf("FAIL: %s\n", name);           \
            return 1;                             \
        }                                         \
        printf("PASS: %s\n", name);               \
    } while (0)

/* Ghi du lieu vao device */
static int write_data(const char *text)
{
    int fd;
    ssize_t n;
    size_t len = strlen(text);

    fd = open("/dev/asgn1", O_WRONLY);

    if (fd < 0)
        return -1;

    n = write(fd, text, len);

    close(fd);

    return n == (ssize_t)len ? 0 : -1;
}

/* Doc du lieu tu device voi offset moi */
static ssize_t read_data(char *buffer, size_t size)
{
    int fd;
    ssize_t n;

    fd = open("/dev/asgn1", O_RDONLY);

    if (fd < 0)
        return -1;

    n = read(fd, buffer, size - 1);

    if (n >= 0)
        buffer[n] = '\0';

    close(fd);

    return n;
}

int main(void)
{
    int fd;
    int mode;
    char buffer[128] = {0};
    char version[ASGN1_VERSION_SIZE] = {0};
    struct asgn1_stats stats;

    fd = open("/dev/asgn1", O_RDWR);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("\n=== ASSIGNMENT 01 DRIVER TEST ===\n\n");

    /* TEST 1: RESET_BUFFER */
    CHECK(ioctl(fd, ASGN1_RESET_BUFFER) == 0,
          "RESET_BUFFER");

    CHECK(ioctl(fd, ASGN1_GET_STATS, &stats) == 0,
          "GET_STATS after reset");

    CHECK(stats.write_count == 0 &&
          stats.read_count == 0 &&
          stats.buffer_len == 0,
          "Counters and buffer reset");

    CHECK(read_data(buffer, sizeof(buffer)) == 0,
          "Buffer empty after reset");

    /* TEST 2: Echo Mode */
    mode = 1;

    CHECK(ioctl(fd, ASGN1_SET_MODE, &mode) == 0,
          "SET_MODE = 1");

    CHECK(write_data("HELLO") == 0,
          "Write Echo Mode");

    CHECK(read_data(buffer, sizeof(buffer)) == 5 &&
          strcmp(buffer, "HELLO") == 0,
          "Read Echo Mode");

    /* TEST 3: Kernel Mode */
    mode = 0;

    CHECK(ioctl(fd, ASGN1_SET_MODE, &mode) == 0,
          "SET_MODE = 0");

    CHECK(write_data("HELLO") == 0,
          "Write Kernel Mode");

    CHECK(read_data(buffer, sizeof(buffer)) == 13 &&
          strcmp(buffer, "[KERNEL]HELLO") == 0,
          "Read Kernel Mode");

    /* TEST 4: GET_STATS */
    CHECK(ioctl(fd, ASGN1_GET_STATS, &stats) == 0,
          "GET_STATS");

    printf("\nDriver Statistics:\n");
    printf("Open count      : %d\n", stats.open_count);
    printf("Write count     : %d\n", stats.write_count);
    printf("Read count      : %d\n", stats.read_count);
    printf("Buffer length   : %d\n", stats.buffer_len);
    printf("Last write size : %d\n", stats.last_write_size);

    CHECK(stats.write_count == 2 &&
          stats.read_count == 2 &&
          stats.buffer_len == 13 &&
          stats.last_write_size == 5,
          "Statistics values");

    /* TEST 5: GET_VERSION */
    CHECK(ioctl(fd, ASGN1_GET_VERSION, version) == 0,
          "GET_VERSION");

    CHECK(memchr(version, '\0', sizeof(version)) != NULL &&
          version[0] != '\0',
          "Valid version string");

    printf("Driver version: %s\n", version);

    /* TEST 6: Invalid Mode */
    mode = 5;
    errno = 0;

    CHECK(ioctl(fd, ASGN1_SET_MODE, &mode) == -1 &&
          errno == EINVAL,
          "Reject invalid mode");

    /* TEST 7: RESET again */
    CHECK(ioctl(fd, ASGN1_RESET_BUFFER) == 0,
          "Final RESET_BUFFER");

    CHECK(read_data(buffer, sizeof(buffer)) == 0,
          "Final buffer empty");

    CHECK(ioctl(fd, ASGN1_GET_STATS, &stats) == 0 &&
          stats.write_count == 0 &&
          stats.read_count == 0 &&
          stats.buffer_len == 0,
          "Final counters reset");

    close(fd);

    printf("\n=== ALL TESTS PASSED ===\n");

    return 0;
}
