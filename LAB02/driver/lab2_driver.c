#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Lab");
MODULE_DESCRIPTION("LAB-02: Simple Character Device Driver");
MODULE_VERSION("1.0");

#define DEVICE_NAME  "lab2"
#define CLASS_NAME   "lab2_class"
#define BUFFER_SIZE  1024
#define DRIVER_MAJOR 240

static int major_number = DRIVER_MAJOR;
static char device_buffer[BUFFER_SIZE];
static size_t buffer_len;
static unsigned int open_count;

static struct class *lab2_class;
static struct device *lab2_device;

static DEFINE_MUTEX(lab2_mutex);

/* Mở thiết bị */
static int lab2_open(struct inode *inode, struct file *file)
{
    unsigned int count;

    mutex_lock(&lab2_mutex);
    count = ++open_count;
    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: device opened (count=%u)\n", count);
    return 0;
}

/* Đóng thiết bị */
static int lab2_release(struct inode *inode, struct file *file)
{
    pr_info("lab2_driver: device closed\n");
    return 0;
}

/* Đọc dữ liệu từ thiết bị */
static ssize_t lab2_read(struct file *file,
                         char __user *buf,
                         size_t len,
                         loff_t *offset)
{
    size_t available;

    if (*offset < 0)
        return -EINVAL;

    if (mutex_lock_interruptible(&lab2_mutex))
        return -ERESTARTSYS;

    if (*offset >= buffer_len) {
        mutex_unlock(&lab2_mutex);
        return 0;
    }

    available = buffer_len - (size_t)*offset;

    if (len > available)
        len = available;

    if (copy_to_user(buf, device_buffer + *offset, len)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }

    *offset += len;

    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: sent %zu bytes to user\n", len);

    return len;
}

/* Ghi dữ liệu vào thiết bị */
static ssize_t lab2_write(struct file *file,
                          const char __user *buf,
                          size_t len,
                          loff_t *offset)
{
    if (len > BUFFER_SIZE - 1)
        len = BUFFER_SIZE - 1;

    if (mutex_lock_interruptible(&lab2_mutex))
        return -ERESTARTSYS;

    if (copy_from_user(device_buffer, buf, len)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }

    buffer_len = len;
    device_buffer[buffer_len] = '\0';

    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: received %zu bytes\n", len);

    return len;
}

/* Đăng ký các thao tác */
static const struct file_operations lab2_fops = {
    .owner   = THIS_MODULE,
    .open    = lab2_open,
    .read    = lab2_read,
    .write   = lab2_write,
    .release = lab2_release,
};

/* Khởi tạo module */
static int __init lab2_init(void)
{
    int ret;

    pr_info("lab2_driver: initializing module\n");

    ret = register_chrdev(major_number, DEVICE_NAME, &lab2_fops);

    if (ret < 0) {
        pr_err("lab2_driver: register_chrdev failed: %d\n", ret);
        return ret;
    }

    lab2_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(lab2_class)) {
        ret = PTR_ERR(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return ret;
    }

    lab2_device = device_create(
        lab2_class,
        NULL,
        MKDEV(major_number, 0),
        NULL,
        DEVICE_NAME
    );

    if (IS_ERR(lab2_device)) {
        ret = PTR_ERR(lab2_device);
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return ret;
    }

    pr_info("lab2_driver: loaded, major=%d\n", major_number);

    return 0;
}

/* Gỡ module */
static void __exit lab2_exit(void)
{
    device_destroy(lab2_class, MKDEV(major_number, 0));
    class_destroy(lab2_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    pr_info("lab2_driver: module unloaded\n");
}

module_init(lab2_init);
module_exit(lab2_exit);
