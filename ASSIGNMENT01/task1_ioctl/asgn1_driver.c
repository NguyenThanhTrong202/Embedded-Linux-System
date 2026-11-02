#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include "asgn1_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Lab");
MODULE_DESCRIPTION("ASSIGNMENT 01: Character Driver with ioctl");
MODULE_VERSION("1.0");

#define DEVICE_NAME  "asgn1"
#define CLASS_NAME   "asgn1_class"
#define BUFFER_SIZE  1024
#define DRIVER_MAJOR 241

static int major_number = DRIVER_MAJOR;
static char device_buffer[BUFFER_SIZE];
static size_t buffer_len;
static unsigned int open_count;
static unsigned int write_count;
static unsigned int read_count;
static unsigned int last_write_size;
/*echo mode*/
static int device_mode = 1;

static struct class *asgn1_class;
static struct device *asgn1_device;

static DEFINE_MUTEX(asgn1_mutex);

/* Mở thiết bị */
static int asgn1_open(struct inode *inode, struct file *file)
{
    unsigned int count;

    mutex_lock(&asgn1_mutex);
    count = ++open_count;
    mutex_unlock(&asgn1_mutex);

    pr_info("asgn1_driver: device opened (count=%u)\n", count);
    return 0;
}

/* Đóng thiết bị */
static int asgn1_release(struct inode *inode, struct file *file)
{
    pr_info("asgn1_driver: device closed\n");
    return 0;
}

/* Đọc dữ liệu từ thiết bị */
static ssize_t asgn1_read(struct file *file,
                         char __user *buf,
                         size_t len,
                         loff_t *offset)
{
    size_t available;

    if (*offset < 0)
        return -EINVAL;

    if (mutex_lock_interruptible(&asgn1_mutex))
        return -ERESTARTSYS;

    if (*offset >= buffer_len) {
        mutex_unlock(&asgn1_mutex);
        return 0;
    }

    available = buffer_len - (size_t)*offset;

    if (len > available)
        len = available;

    if (copy_to_user(buf, device_buffer + *offset, len)) {
        mutex_unlock(&asgn1_mutex);
        return -EFAULT;
    }

    *offset += len;

    if (len > 0)
	read_count++;

    mutex_unlock(&asgn1_mutex);

    pr_info("asgn1_driver: sent %zu bytes to user\n", len);

    return len;
}

/* Ghi du lieu vao driver theo che do hien tai */
static ssize_t asgn1_write(struct file *file,
                          const char __user *buf,
                          size_t len,
                          loff_t *offset)
{
    const char prefix[] = "[KERNEL]";
    char temp[BUFFER_SIZE];

    size_t prefix_len = 0;
    size_t user_len;
    int mode_used;

    /* Khong xu ly neu khong co du lieu */
    if (len == 0)
        return 0;

    /* Khoa mutex */
    if (mutex_lock_interruptible(&asgn1_mutex))
        return -ERESTARTSYS;

    mode_used = device_mode;

    /* Mode 0: Them tien to KERNEL */
    if (mode_used == 0) {
        prefix_len = sizeof(prefix) - 1;
        memcpy(temp, prefix, prefix_len);
    }

    /* Gioi han du lieu trong BUFFER_SIZE */
    user_len = len;

    if (user_len > BUFFER_SIZE - 1 - prefix_len)
        user_len = BUFFER_SIZE - 1 - prefix_len;

    /* Nhan du lieu tu userspace */
    if (copy_from_user(temp + prefix_len, buf, user_len)) {
        mutex_unlock(&asgn1_mutex);
        return -EFAULT;
    }

    /* Luu du lieu vao buffer */
    buffer_len = prefix_len + user_len;

    memcpy(device_buffer, temp, buffer_len);
    device_buffer[buffer_len] = '\0';

    /* Cap nhat thong ke */
    last_write_size = (unsigned int)user_len;
    write_count++;

    /* Mo khoa mutex */
    mutex_unlock(&asgn1_mutex);

    pr_info("asgn1_driver: write mode=%d, bytes=%zu\n",
            mode_used, user_len);

    return user_len;
}


/* Xử lý các lệnh ioctl */

static long asgn1_ioctl(struct file *file,
                       unsigned int cmd,
                       unsigned long arg)
{
    switch (cmd) {

    case ASGN1_RESET_BUFFER:

        /* 1. Khoa mutex truoc khi thay doi du lieu */
        if (mutex_lock_interruptible(&asgn1_mutex))
            return -ERESTARTSYS;

        /* 2. Xoa du lieu trong buffer */
        memset(device_buffer, 0, sizeof(device_buffer));

        /* 3. Dat do dai buffer ve 0 */
        buffer_len = 0;

        /* 4. Reset bo dem */
        write_count = 0;
        read_count = 0;

        /* 5. Mo khoa mutex */
        mutex_unlock(&asgn1_mutex);

        /* 6. Ghi thong bao */
        pr_info("asgn1_driver: buffer reset successfully\n");

        return 0;

        case ASGN1_GET_STATS: {
        struct asgn1_stats stats;

        /* Khoa mutex */
        if (mutex_lock_interruptible(&asgn1_mutex))
            return -ERESTARTSYS;

        /* Lay thong tin tu driver */
        stats.open_count = (int)open_count;
        stats.write_count = (int)write_count;
        stats.read_count = (int)read_count;
        stats.buffer_len = (int)buffer_len;
        stats.last_write_size = (int)last_write_size;

        /* Mo khoa mutex */
        mutex_unlock(&asgn1_mutex);

        /* Gui thong tin ve userspace */
        if (copy_to_user((void __user *)arg,
                         &stats,
                         sizeof(stats)))
            return -EFAULT;

        pr_info("asgn1_driver: GET_STATS completed\n");

        return 0;
    }

case ASGN1_SET_MODE: {
    int new_mode;

    /* 1. Nhan mode tu userspace */
    if (copy_from_user(&new_mode,
                       (void __user *)arg,
                       sizeof(new_mode)))
        return -EFAULT;

    /* 2. Kiem tra mode hop le */
    if (new_mode != 0 && new_mode != 1)
        return -EINVAL;

    /* 3. Khoa mutex */
    if (mutex_lock_interruptible(&asgn1_mutex))
        return -ERESTARTSYS;

    /* 4. Cap nhat che do */
    device_mode = new_mode;

    /* 5. Mo khoa mutex */
    mutex_unlock(&asgn1_mutex);

    /* 6. Ghi kernel log */
    pr_info("asgn1_driver: SET_MODE = %d\n", new_mode);

    return 0;
}

case ASGN1_GET_VERSION: {

    /* Chuoi version cua driver */
    char version[ASGN1_VERSION_SIZE] =
        "asgn1_driver v1.0";

    /* Gui version tu kernel ve userspace */
    if (copy_to_user((void __user *)arg,
                     version,
                     sizeof(version)))
        return -EFAULT;

    pr_info("asgn1_driver: GET_VERSION completed\n");

    return 0;
}

    default:
        return -ENOTTY;
    }
}

/* Đăng ký các thao tác */
static const struct file_operations asgn1_fops = {
    .owner   = THIS_MODULE,
    .open    = asgn1_open,
    .read    = asgn1_read,
    .write   = asgn1_write,
    .release = asgn1_release,
    .unlocked_ioctl = asgn1_ioctl,
};

/* Khởi tạo module */
static int __init asgn1_init(void)
{
    int ret;

    pr_info("asgn1_driver: initializing module\n");

    ret = register_chrdev(major_number, DEVICE_NAME, &asgn1_fops);

    if (ret < 0) {
        pr_err("asgn1_driver: register_chrdev failed: %d\n", ret);
        return ret;
    }

    asgn1_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(asgn1_class)) {
        ret = PTR_ERR(asgn1_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return ret;
    }

    asgn1_device = device_create(
        asgn1_class,
        NULL,
        MKDEV(major_number, 0),
        NULL,
        DEVICE_NAME
    );

    if (IS_ERR(asgn1_device)) {
        ret = PTR_ERR(asgn1_device);
        class_destroy(asgn1_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return ret;
    }

    pr_info("asgn1_driver: loaded, major=%d\n", major_number);

    return 0;
}

/* Gỡ module */
static void __exit asgn1_exit(void)
{
    device_destroy(asgn1_class, MKDEV(major_number, 0));
    class_destroy(asgn1_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    pr_info("asgn1_driver: module unloaded\n");
}

module_init(asgn1_init);
module_exit(asgn1_exit);
