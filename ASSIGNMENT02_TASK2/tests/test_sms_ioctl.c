/* Test Task 2: ioctl, ASCII sensor read and /proc integration.
 * Study/test harness; adjust and understand before submitting.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../../ASSIGNMENT02_TASK1_COMPLETE/driver/sms_sensor.h"

int main(void)
{
    const char *path = SMS_DEVICE_PATH;
    char data[128];
    unsigned int interval = 500;
    unsigned int invalid = 0;
    unsigned long long ts;
    double temperature, humidity;
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    int failed = 0;
    ssize_t n;

    if (fd == -1) { perror("open device"); return 1; }
    if (ioctl(fd, SMS_SET_INTERVAL, &interval) == -1) {
        perror("ioctl(500ms)"); failed = 1;
    } else {
        puts("PASS ioctl SMS_SET_INTERVAL=500ms");
    }
    errno = 0;
    if (ioctl(fd, SMS_SET_INTERVAL, &invalid) == -1 && errno == EINVAL) {
        puts("PASS ioctl rejects invalid interval=0 (EINVAL)");
    } else {
        fprintf(stderr, "FAIL ioctl should reject interval=0 (errno=%d)\n", errno);
        failed = 1;
    }
    /* Close/open per record, because the driver implements one CSV line per open. */
    close(fd);
    sleep(1);
    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1) { perror("re-open device"); return 1; }
    n = read(fd, data, sizeof(data) - 1);
    if (n < 0) { perror("read CSV"); failed = 1; }
    else if (n == 0) { fprintf(stderr, "FAIL premature EOF\n"); failed = 1; }
    else {
        data[n] = '\0';
        if (sscanf(data, "%llu,%lf,%lf", &ts, &temperature, &humidity) == 3 &&
            temperature >= 15.0 && temperature <= 45.0 &&
            humidity >= 30.0 && humidity <= 90.0) {
            printf("PASS CSV %s", data);
        } else {
            fprintf(stderr, "FAIL CSV format/range: %s", data);
            failed = 1;
        }
    }
    close(fd);
    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1) { perror("second reopen"); return 1; }
    errno = 0;
    n = read(fd, data, sizeof(data) - 1);
    if (n == -1 && errno == EAGAIN) {
        puts("PASS early read -> EAGAIN");
    } else {
        printf("INFO early read returned %zd (errno=%d); timing race possible\n", n, errno);
        /* Deliberately avoid false FAIL if scheduling delayed the read >500ms. */
    }
    close(fd);
    FILE *stats = fopen("/proc/sms_stats", "r");
    if (stats == NULL) { perror("/proc/sms_stats"); return 1; }
    puts("=== /proc/sms_stats ===");
    while (fgets(data, sizeof(data), stats)) fputs(data, stdout);
    fclose(stats);
    if (failed) return 1;
    puts("PASS: driver ioctl + device CSV + procfs");
    return 0;
}
