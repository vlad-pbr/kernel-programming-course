// km.c

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

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

static int major_num;
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

static int __init km_init(void) {

    // fill buffer with message
    strncpy(msg_buffer, MSG, MSG_BUFFER_LEN);
    msg_ptr = msg_buffer;

    // register character device
    major_num = register_chrdev(0, DEVICE_NAME, &file_opts);
    printk(KERN_INFO "%s: module loaded, device major number %d\n", DEVICE_NAME, major_num);

    return 0;
};

static void __exit km_exit(void) {

    // unregister character device
    unregister_chrdev(major_num, DEVICE_NAME);

    printk(KERN_INFO "%s: module unloaded\n", DEVICE_NAME);
};

module_init(km_init);
module_exit(km_exit);