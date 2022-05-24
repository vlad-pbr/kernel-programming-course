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
#define KM_PAGE_SIZE 4096

// ioctl commands
#define GET_PHYS_MEM _IOWR(234, 100, char*)
#define GET_PHYS_MEM2 _IOWR(234, 200, unsigned long*)
#define GET_CR3 _IOR(234, 101, unsigned long*)
#define GET_TASK_STRUCT _IOR(234, 102, unsigned long*)

#define GET_CR0 _IOR(234, 110, unsigned long*)
#define GET_CR4 _IOR(234, 111, unsigned long*)

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
            char buffer[KM_PAGE_SIZE];
            char frame_num_buffer[sizeof(u_int64_t)];
            // char segment_buffer[sizeof(int32_t)];
            u_int64_t frame_num;
            unsigned long frame_physical_address;
            int byte_index;

            // copy frame number bytes from user space buffer
            _ = copy_from_user(frame_num_buffer, (char*)arg, sizeof(u_int64_t));

            // case to frame number integer
            frame_num = *((u_int64_t*)frame_num_buffer);

            // calculate physical address of frame
            frame_physical_address = frame_num * KM_PAGE_SIZE;

            // read and store each byte
            for (byte_index = 0 ; byte_index < KM_PAGE_SIZE; byte_index++) {

                // physical memory is not directly understood by CPU
                // instead it keeps a mapping of physical memory
                // so we access the memory after translation
                buffer[byte_index] = *(char*)phys_to_virt((phys_addr_t)frame_physical_address++);
            }

            // store buffer to user space
            _ = copy_to_user((char*)arg, buffer, KM_PAGE_SIZE);

            break;

        // handle physical memory reading
        case GET_PHYS_MEM2:

            // define variables
            unsigned long addr;
            addr = 3;

            printk(KERN_NOTICE "addr: %lu\n", addr);

            // copy frame number bytes from user space buffer
            _ = copy_from_user(&addr, (unsigned long*)arg, sizeof(unsigned long));

            printk(KERN_NOTICE "phys_addr: %lu\n", addr);

            phys_addr_t byte = addr;

            printk(KERN_NOTICE "phys_addr_t: %lu\n", byte);

            char* after = phys_to_virt(byte);

            printk(KERN_NOTICE "after: %lu\n", after);

            printk(KERN_NOTICE "value: %lu\n", *(after++));
            printk(KERN_NOTICE "value: %lu\n", *(after++));
            printk(KERN_NOTICE "value: %lu\n", *(after++));
            printk(KERN_NOTICE "value: %lu\n", *(after++));
            printk(KERN_NOTICE "value: %lu\n", *(after++));
            printk(KERN_NOTICE "value: %lu\n", *(after++));
            printk(KERN_NOTICE "value: %lu\n", *(after++));
            printk(KERN_NOTICE "value: %lu\n", *(after++));

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

        // handle CR0 register
        case GET_CR0:

            // store value of CR0 register
            unsigned long cr0;
            __asm__ __volatile__(
                "mov %%cr0, %%rax\n\t"
                "mov %%rax, %0\n\t"
                : "=m" (cr0) : : "%rax"
            );

            // copy value to user space memory
            _ = copy_to_user((unsigned long*)arg, &cr0, sizeof(cr0));

            break;

        // handle CR4 register
        case GET_CR4:

            // store value of CR4 register
            unsigned long cr4;
            __asm__ __volatile__(
                "mov %%cr4, %%rax\n\t"
                "mov %%rax, %0\n\t"
                : "=m" (cr4) : : "%rax"
            );

            // copy value to user space memory
            _ = copy_to_user((unsigned long*)arg, &cr4, sizeof(cr4));

            break;

        // handle task struct address
        case GET_TASK_STRUCT:

            // define variables
            struct task_struct *task = current;

            // fill user space buffer with virtual address of task struct
            _ = copy_to_user((unsigned long*)arg, &task, sizeof(task));

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