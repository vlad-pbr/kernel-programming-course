// km.c

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/syscalls.h>
#include <linux/ioctl.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vlad Poberezhny");
MODULE_DESCRIPTION("Kernel programming course assignment 3, College of Management Academic Studies");
MODULE_VERSION("0.1");

#define DEVICE_NAME "km"
#define BUFFER_LEN 4096

// ioctl commands
#define GET_PHYS_MEM _IOWR(234, 100, char*)
#define GET_CR3 _IOR(234, 101, unsigned long*)

// device function prototypes
static long device_ioctl(struct file *, unsigned int, unsigned long);

// device related variables
struct class *km_cl;
static dev_t km_dev;
static struct cdev km_cdev;

// device operations struct
static struct file_operations file_opts = {
    .unlocked_ioctl = device_ioctl
};

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

    // define variables
    unsigned long _; // return value placeholder to ignore warnings

    switch (cmd) {

        // handle physical memory reading
        case GET_PHYS_MEM:

            // define variables
            char buffer[BUFFER_LEN];
            char segment_buffer[sizeof(int32_t)];
            int32_t segment;
            int byte_index;

            // copy segment number bytes from user space buffer
            _ = copy_from_user(segment_buffer, (char*)arg, sizeof(segment_buffer));

            // parse segment number and calculate address of segment
            segment = (*((int32_t*)segment_buffer) - 1) * BUFFER_LEN;

            // read and store each byte
            for (byte_index = 0 ; byte_index < BUFFER_LEN; byte_index++) {

                // physical memory is not understood by CPU
                // instead it keeps a mapping of physical memory
                // so we access the memory after translation
                buffer[byte_index] = *(char*)phys_to_virt((phys_addr_t)segment++);
            }

            // send buffer to user space
            _ = copy_to_user((char*)arg, buffer, BUFFER_LEN);

            break;

        // handle CR3 register
        case GET_CR3:

            // store value of CR3 register
            unsigned long cr3;
            __asm__ __volatile__(
                "mov %%cr3, %%rax\n\t"
                "mov %%rax, %0\n\t"
                : "=m" (cr3) : : "%rax"
            );

            // copy value to user space memory
            _ = copy_to_user((unsigned long*)arg, &cr3, sizeof(cr3));

            break;

    }

    return 0;
}

static int km_uevent(struct device *dev, struct kobj_uevent_env *env) {

    // grant read-write permissions for device to all users
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int __init km_init(void) {

    // register character device
    alloc_chrdev_region(&km_dev, 0, 1, DEVICE_NAME);
    km_cl = class_create(THIS_MODULE, "chardrv");
    km_cl->dev_uevent = km_uevent;
    device_create(km_cl, NULL, km_dev, NULL, DEVICE_NAME);
    cdev_init(&km_cdev, &file_opts);
    cdev_add(&km_cdev, km_dev, 1);

    return 0;
};

static void __exit km_exit(void) {

    // unregister character device
    device_destroy(km_cl, km_dev);
    cdev_del(&km_cdev);
    class_destroy(km_cl);
    unregister_chrdev_region(km_dev, 1);
};

module_init(km_init);
module_exit(km_exit);