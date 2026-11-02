/*
 * sms_sensor_driver.c - Simulated temperature/humidity misc device.
 * Date: 2026-11-02
 * Author: [update author and student ID if required]
 *
 * Uses integer tenths throughout: floating point is not supported in kernel.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/random.h>
#include <linux/timekeeping.h>
#include <linux/math64.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include "sms_sensor.h"

#define SMS_DEFAULT_INTERVAL_MS 100U
#define SMS_MAX_INTERVAL_MS   60000U

struct sms_file_sample {
    char csv[96];
    size_t length;
    bool ready;
};

static DEFINE_MUTEX(sensor_lock);
static struct proc_dir_entry *sensor_proc_entry;
static u64 module_start_ms;
static u64 last_sample_ms;
static u64 read_count;
static unsigned int last_temp_tenths;
static unsigned int last_hum_tenths;
static unsigned int min_interval_ms = SMS_DEFAULT_INTERVAL_MS;

/* Use CLOCK_MONOTONIC in ms; avoids ktime_get_ms() declaration issues. */
static u64 sms_now_ms(void)
{
    return div_u64(ktime_get_ns(), 1000000U);
}

/* Allocate per-open storage so partial reads keep one CSV sample. */
static int sms_open(struct inode *inode, struct file *file)
{
    struct sms_file_sample *state;
    int rc;

    state = kzalloc(sizeof(*state), GFP_KERNEL);
    if (!state)
        return -ENOMEM;
    rc = nonseekable_open(inode, file);
    if (rc) {
        kfree(state);
        return rc;
    }
    file->private_data = state;
    return 0;
}

/* Return exactly one CSV record per open(), then EOF. */
static ssize_t sms_read(struct file *file, char __user *buf,
                        size_t len, loff_t *ppos)
{
    struct sms_file_sample *state = file->private_data;
    u64 now_ms;
    unsigned int t10, h10;
    ssize_t ret;

    if (!state || *ppos < 0)
        return -EINVAL;
    if (len == 0)
        return 0;

    if (!state->ready) {
        if (mutex_lock_interruptible(&sensor_lock))
            return -ERESTARTSYS;
        now_ms = sms_now_ms();
        if (read_count && (now_ms - last_sample_ms) < min_interval_ms) {
            mutex_unlock(&sensor_lock);
            return -EAGAIN;
        }
        t10 = 150U + get_random_u32() % 301U; /* 15.0 .. 45.0 C */
        h10 = 300U + get_random_u32() % 601U; /* 30.0 .. 90.0 % */
        state->length = scnprintf(state->csv, sizeof(state->csv),
                                  "%llu,%u.%u,%u.%u\n",
                                  (unsigned long long)now_ms,
                                  t10 / 10, t10 % 10,
                                  h10 / 10, h10 % 10);
        state->ready = true;
        last_sample_ms = now_ms;
        last_temp_tenths = t10;
        last_hum_tenths = h10;
        read_count++;
        mutex_unlock(&sensor_lock);
    }

    ret = simple_read_from_buffer(buf, len, ppos, state->csv, state->length);
    return ret;
}

/* Configure minimum time (ms) allowed between generated samples. */
static long sms_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int interval;
    (void)file;

    if (cmd != SMS_SET_INTERVAL)
        return -ENOTTY;
    if (copy_from_user(&interval, (const void __user *)arg, sizeof(interval)))
        return -EFAULT;
    if (interval < 1U || interval > SMS_MAX_INTERVAL_MS)
        return -EINVAL;

    if (mutex_lock_interruptible(&sensor_lock))
        return -ERESTARTSYS;
    min_interval_ms = interval;
    mutex_unlock(&sensor_lock);
    pr_info("sms_sensor: sampling interval=%u ms\n", interval);
    return 0;
}

/* Free storage allocated by sms_open(). */
static int sms_release(struct inode *inode, struct file *file)
{
    (void)inode;
    kfree(file->private_data);
    file->private_data = NULL;
    return 0;
}

static const struct file_operations sms_fops = {
    .owner          = THIS_MODULE,
    .open           = sms_open,
    .read           = sms_read,
    .unlocked_ioctl = sms_ioctl,
    .release        = sms_release,
};

static struct miscdevice sms_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = SMS_DEVICE_NAME,
    .fops  = &sms_fops,
    .mode  = 0666,
};

/* Show the counters and last sensor values under /proc/sms_stats. */
static int sms_proc_show(struct seq_file *m, void *unused)
{
    u64 count, elapsed;
    unsigned int t10, h10, interval;
    (void)unused;

    mutex_lock(&sensor_lock);
    count = read_count;
    t10 = last_temp_tenths;
    h10 = last_hum_tenths;
    interval = min_interval_ms;
    elapsed = div_u64(sms_now_ms() - module_start_ms, 1000U);
    mutex_unlock(&sensor_lock);

    seq_printf(m, "read_count: %llu\n", (unsigned long long)count);
    seq_printf(m, "last_temperature: %u.%u\n", t10 / 10, t10 % 10);
    seq_printf(m, "last_humidity: %u.%u\n", h10 / 10, h10 % 10);
    seq_printf(m, "driver_uptime_seconds: %llu\n", (unsigned long long)elapsed);
    seq_printf(m, "interval_ms: %u\n", interval);
    return 0;
}

static int sms_proc_open(struct inode *inode, struct file *file)
{
    (void)inode;
    return single_open(file, sms_proc_show, NULL);
}

static const struct proc_ops sms_proc_ops = {
    .proc_open    = sms_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init sms_init(void)
{
    int rc;
    module_start_ms = sms_now_ms();
    rc = misc_register(&sms_misc);
    if (rc)
        return rc;
    sensor_proc_entry = proc_create(SMS_PROC_NAME, 0444, NULL, &sms_proc_ops);
    if (!sensor_proc_entry) {
        misc_deregister(&sms_misc);
        return -ENOMEM;
    }
    pr_info("sms_sensor: loaded major=10 minor=%d\n", sms_misc.minor);
    return 0;
}

static void __exit sms_exit(void)
{
    proc_remove(sensor_proc_entry);
    misc_deregister(&sms_misc);
    pr_info("sms_sensor: unloaded\n");
}

module_init(sms_init);
module_exit(sms_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student - update with name and student ID if required");
MODULE_DESCRIPTION("Synthetic temperature and humidity sensor for ASM2");
MODULE_VERSION("1.0");
