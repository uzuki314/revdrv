#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/version.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("hoshinomori uzuki , Taiwan");
MODULE_DESCRIPTION("Reverse driver");
MODULE_VERSION("0.1");

#define DEV_REVERSE_NAME "reverse"

static dev_t rev_dev = 0;
static struct class *rev_class;
static DEFINE_MUTEX(rev_mutex);
static int major = 0, minor = 0;

typedef struct {
    struct list_head list;
    size_t size;
    char str[];
} rev_data;
static LIST_HEAD(rev_head);
static size_t rev_list_size = 0;

/**
 * reverse_str() - Reverse the given string inplace
 * @str:     char pointer to the beginning of the string
 * @size:    string length
 */
static void reverse_str(char *str, size_t size)
{
    char *p1, *p2;
    for (p1 = str, p2 = str + size - 1; p2 > p1; ++p1, --p2) {
        *p1 ^= *p2;
        *p2 ^= *p1;
        *p1 ^= *p2;
    }
}

/**
 * rev_list_add() - Add buf to the head of the reverse list.
 * @head:     list_head of the reverse list
 * @buf:      user write buffer
 * @size:     buffer size
 * Return: 0 if success, <0 indicates that an error was encountered.
 */
static long long rev_list_add(struct list_head *head,
                              const char *buf,
                              size_t size)
{
    int rc = 0;
    rev_data *entry = kmalloc(sizeof(rev_data) + size, GFP_KERNEL);
    if (!entry) {
        printk(KERN_ERR "Failed to kmalloc: No memory\n");
        return -ENOMEM;
    }

    rc = copy_from_user(entry->str, buf, size);
    if (rc) {
        printk(KERN_ERR "Failed to copy %d bytes from user space\n", rc);
        return -EFAULT;
    }

    entry->size = size;

    reverse_str(entry->str, size);
    printk(KERN_INFO "Reverse str: %.*s\n", (int) entry->size, entry->str);

    list_add(&entry->list, head);
    rev_list_size += size;

    return 0;
}

/**
 * rev_list_free() - Free all memory in reverse list.
 * @head:     list_head of the reverse list
 */
static void rev_list_free(struct list_head *head)
{
    printk(KERN_INFO "Clear data\n");
    struct list_head *listptr, *tmp;
    rev_data *entry;

    list_for_each_safe (listptr, tmp, head) {
        entry = list_entry(listptr, rev_data, list);
        list_del(listptr);
        kfree(entry);
    }
    rev_list_size = 0;
}

static int rev_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&rev_mutex)) {
        printk(KERN_ALERT "revdrv is in use\n");
        return -EBUSY;
    }

    if (file->f_flags & O_TRUNC) {
        // clear all content
        if (!list_empty(&rev_head)) {
            rev_list_free(&rev_head);
        }
    }
    return 0;
}

static int rev_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&rev_mutex);
    return 0;
}

static ssize_t rev_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    loff_t loffset = *offset;
    ssize_t total_bytes_read = 0;
    rev_data *entry;

    if (list_empty(&rev_head)) {
        return 0;
    }

    list_for_each_entry (entry, &rev_head, list) {
        if (loffset >= entry->size) {
            loffset -= entry->size;
        } else {
            size_t bytes_to_copy = min(size, entry->size - loffset);
            if (copy_to_user(buf + total_bytes_read, entry->str + loffset,
                             bytes_to_copy)) {
                return -EFAULT;
            }

            total_bytes_read += bytes_to_copy;
            *offset += bytes_to_copy;
            loffset = 0;

            if (total_bytes_read >= size) {
                break;
            }
        }
    }
    return total_bytes_read;
}

static ssize_t rev_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    loff_t loffset = *offset;
    ssize_t total_bytes_write = 0;
    rev_data *entry;
    int rc = 0;

    if (list_empty(&rev_head) || (file->f_flags & O_APPEND)) {
        rc = rev_list_add(&rev_head, buf, size);
        if (rc < 0)
            return rc;
        *offset += size;
        return size;
    }

    list_for_each_entry_reverse(entry, &rev_head, list)
    {
        if (loffset >= entry->size) {
            loffset -= entry->size;
        } else {
            size_t bytes_to_copy = min(size, entry->size - loffset);
            if (copy_from_user(
                    entry->str + entry->size - loffset - bytes_to_copy,
                    buf + total_bytes_write, bytes_to_copy)) {
                return -EFAULT;
            }
            reverse_str(entry->str + entry->size - loffset - bytes_to_copy,
                        bytes_to_copy);
            printk(KERN_INFO "Reverse str: %.*s\n", (int) bytes_to_copy,
                   entry->str + entry->size - loffset - bytes_to_copy);

            total_bytes_write += bytes_to_copy;
            size -= bytes_to_copy;
            *offset += bytes_to_copy;
            loffset = 0;

            if (total_bytes_write >= size) {
                break;
            }
        }
    }

    if (size) {
        rc = rev_list_add(&rev_head, buf + total_bytes_write, size);
        if (rc < 0)
            return rc;
        total_bytes_write += size;
        *offset += size;
    }
    return total_bytes_write;
}

static loff_t rev_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = rev_list_size + offset;
        break;
    }

    if (new_pos > rev_list_size)
        new_pos = rev_list_size;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations rev_fops = {
    .owner = THIS_MODULE,
    .read = rev_read,
    .write = rev_write,
    .open = rev_open,
    .release = rev_release,
    .llseek = rev_device_lseek,
};

static int __init init_rev_dev(void)
{
    int rc = 0;
    mutex_init(&rev_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = major = register_chrdev(major, DEV_REVERSE_NAME, &rev_fops);
    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev\n");
        rc = -2;
        goto failed_cdev;
    }
    rev_dev = MKDEV(major, minor);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    rev_class = class_create(DEV_REVERSE_NAME);
#else
    rev_class = class_create(THIS_MODULE, DEV_REVERSE_NAME);
#endif
    if (!rev_class) {
        printk(KERN_ALERT "Failed to create device class\n");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(rev_class, NULL, rev_dev, NULL, DEV_REVERSE_NAME)) {
        printk(KERN_ALERT "Failed to create device\n");
        rc = -4;
        goto failed_device_create;
    }
    return 0;
failed_device_create:
    class_destroy(rev_class);
failed_class_create:
failed_cdev:
    unregister_chrdev(major, DEV_REVERSE_NAME);
    return rc;
}

static void __exit exit_rev_dev(void)
{
    if (!list_empty(&rev_head)) {
        rev_list_free(&rev_head);
    }
    mutex_destroy(&rev_mutex);
    device_destroy(rev_class, rev_dev);
    class_destroy(rev_class);
    unregister_chrdev(major, DEV_REVERSE_NAME);
}

module_init(init_rev_dev);
module_exit(exit_rev_dev);
