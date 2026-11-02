#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>

#define DEVICE_NAME "counter"

static atomic_t counter = ATOMIC_INIT(0);

static atomic_t total_reads = ATOMIC_INIT(0);

static atomic_t total_writes = ATOMIC_INIT(0);

struct counter_file_state {
    char text[32];
    size_t length;
    bool ready;
    bool counted;
};

/* Cap phat trang thai cho moi lan mo device */
static int counter_open(struct inode *inode, struct file *file)
{
    struct counter_file_state *state;

    state = kzalloc(sizeof(*state), GFP_KERNEL);

    if (!state)
        return -ENOMEM;

    file->private_data = state;

    return 0;
}

/* Doc gia tri counter va tu dong tang */
static ssize_t counter_read(struct file *file,
                            char __user *buf,
                            size_t count,
                            loff_t *ppos)
{
    struct counter_file_state *state = file->private_data;
    ssize_t ret;

    if (*ppos < 0)
        return -EINVAL;

    if (count == 0)
        return 0;

    /* Chuan bi gia tri cho lan doc dau tien */
    if (!state->ready) {
        int value = atomic_fetch_inc(&counter);

        state->length = scnprintf(
            state->text,
            sizeof(state->text),
            "%d\n",
            value
        );

        state->ready = true;
    }

    /* Doc du lieu, co xu ly file offset */
    ret = simple_read_from_buffer(
        buf,
        count,
        ppos,
        state->text,
        state->length
    );

    /* Chi thong ke mot lan cho moi gia tri duoc doc */
    if (ret > 0 && !state->counted) {
        atomic_inc(&total_reads);
        state->counted = true;
    }

    return ret;
}

/* Ghi mot so nguyen vao counter */
static ssize_t counter_write(struct file *file,
                             const char __user *buf,
                             size_t count,
                             loff_t *ppos)
{
    char input[32];
    int value;

    if (count == 0)
        return 0;

    if (count >= sizeof(input))
        return -EINVAL;

    /* Nhan chuoi tu userspace */
    if (copy_from_user(input, buf, count))
        return -EFAULT;

    input[count] = '\0';

    /* Chuyen chuoi thanh so nguyen */
    if (kstrtoint(input, 10, &value))
        return -EINVAL;

    /* Khong chap nhan gia tri am */
    if (value < 0)
        return -EINVAL;

    /* Cap nhat counter */
    atomic_set(&counter, value);

    /* Tang so lan ghi thanh cong */
    atomic_inc(&total_writes);

    pr_info("counter_driver: counter set to %d\n", value);

    return count;
}

/* Giai phong trang thai khi dong device */
static int counter_release(struct inode *inode,
                           struct file *file)
{
    kfree(file->private_data);

    return 0;
}

static const struct file_operations counter_fops = {
    .owner   = THIS_MODULE,
    .open    = counter_open,
    .read    = counter_read,
    .write   = counter_write,
    .release = counter_release,
    .llseek  = no_llseek,
};

/* PROCFS: /proc/counter_info */

static struct proc_dir_entry *counter_proc_entry;

static int counter_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Counter value: %d\n", atomic_read(&counter));
    seq_printf(m, "Total reads: %d\n", atomic_read(&total_reads));
    seq_printf(m, "Total writes: %d\n", atomic_read(&total_writes));

    return 0;
}

static int counter_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, counter_proc_show, NULL);
}

static const struct proc_ops counter_proc_ops = {
    .proc_open    = counter_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* MISC DEVICE */

static struct miscdevice counter_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &counter_fops,
    .mode  = 0666,
};

/* MODULE INITIALIZATION */

static int __init counter_init(void)
{
    int ret;

    ret = misc_register(&counter_misc_device);

    if (ret) {
        pr_err("counter_driver: misc_register failed\n");
        return ret;
    }

    counter_proc_entry = proc_create(
        "counter_info",
        0444,
        NULL,
        &counter_proc_ops
    );

    if (!counter_proc_entry) {
        misc_deregister(&counter_misc_device);
        return -ENOMEM;
    }

    pr_info("counter_driver: loaded, major=10 minor=%d\n",
            counter_misc_device.minor);

    return 0;
}

/* MODULE CLEANUP */

static void __exit counter_exit(void)
{
    proc_remove(counter_proc_entry);

    misc_deregister(&counter_misc_device);

    pr_info("counter_driver: unloaded\n");
}

module_init(counter_init);
module_exit(counter_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Thanh Trong - replace with MSSV");
MODULE_DESCRIPTION("ASSIGNMENT01 Task2 Misc Counter Driver");
MODULE_VERSION("1.0");

