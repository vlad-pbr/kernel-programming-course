// km.c

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/syscalls.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vlad Poberezhny");
MODULE_DESCRIPTION("Kernel programming course assignment 3, College of Management Academic Studies");
MODULE_VERSION("0.1");

#define DEVICE_NAME "km"
#define MSG "Hello, world!\n"
#define MSG_BUFFER_LEN 15

// device function prototypes
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

// device related variables
struct class *km_cl;
static dev_t km_dev;
static struct cdev km_cdev;

// global variables
static int device_open_count = 0;
static char msg_buffer[MSG_BUFFER_LEN];
static char *msg_ptr;

// device operations struct
static struct file_operations file_opts = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t *offset) {

    int bytes_read = 0;

    // if pointer already at the end - jump back to start
    if (*msg_ptr == 0) {
        msg_ptr = msg_buffer;
    }

    // put message to buffer
    while (len && *msg_ptr) {

        // read from buffer in user space
        put_user(*(msg_ptr++), buffer++);
        len--;
        bytes_read++;
    }

    return bytes_read;
}

static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset) {

    // device is read only for now
    return -EINVAL;
}

static int device_open(struct inode *inode, struct file *file) {

    // return busy if device is open
    if (device_open_count) {
        return -EBUSY;
    }

    // open device by incrementing the open count
    device_open_count++;
    try_module_get(THIS_MODULE);
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {

    // release device by decrementing the open count
    device_open_count--;
    module_put(THIS_MODULE);
    return 0;
}

static int km_uevent(struct device *dev, struct kobj_uevent_env *env) {

    // grant read-write permissions for device to all users
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int __init km_init(void) {

    // fill buffer with message
    strncpy(msg_buffer, MSG, MSG_BUFFER_LEN);
    msg_ptr = msg_buffer;

    // register character device
    alloc_chrdev_region(&km_dev, 0, 1, DEVICE_NAME);
    km_cl = class_create(THIS_MODULE, "chardrv");
    km_cl->dev_uevent = km_uevent;
    device_create(km_cl, NULL, km_dev, NULL, DEVICE_NAME);
    cdev_init(&km_cdev, &file_opts);
    cdev_add(&km_cdev, km_dev, 1);

    printk(KERN_INFO "%s: module loaded, device created at /dev/%s\n", DEVICE_NAME, DEVICE_NAME);

    return 0;
};

static void __exit km_exit(void) {

    // unregister character device
    device_destroy(km_cl, km_dev);
    cdev_del(&km_cdev);
    class_destroy(km_cl);
    unregister_chrdev_region(km_dev, 1);

    printk(KERN_INFO "%s: module unloaded\n", DEVICE_NAME);
};

module_init(km_init);
module_exit(km_exit);