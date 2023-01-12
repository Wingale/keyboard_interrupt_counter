/* module creates a device that monitors
 * keyboard interrupts and reports on them */

#include<linux/kernel.h>        // required for working with kernel
#include<linux/module.h>        // required for all modules
#include<linux/init.h>          // enables __init and __exit
#include<linux/cdev.h>          // cdev interface, for registering character devices
#include<linux/device.h>        // tools for devices
#include<linux/fs.h>            // file ops
#include<linux/interrupt.h>     // interrupt request handler
#include<linux/ktime.h>         // provides time related utilities

#include"keyboard_interrupt_counter.h"


MODULE_LICENSE("GPL");

// controlling exclusive access to the device via atomic operations
enum {
        CDEV_NOT_USED = 0,
        CDEV_EXCLUSIVE_OPEN = 1,
};
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static struct class *kic_cls;
static unsigned int kic_interrupt_count = 0;    // count of interrupts
static ktime_t kic_time = 0;    // time and date of last counter reset

// prototypes for purpose-specific functions
static irqreturn_t kic_increment(int, void *);
static void kic_reset(void);


        // file operations //
// maintains reference count, invoked by ioctl protocol
static int kic_device_open(struct inode *inode, struct file *file)
{
        pr_info("KIC: Device Opened.\n");
        try_module_get(THIS_MODULE);
        return 0;

}

// maintains reference count, invoked by ioctl protocol
static int kic_device_release(struct inode *inode, struct file *file)
{
        pr_info("KIC: Device Freed.\n");
        module_put(THIS_MODULE);
        return 0;
}

/*
 * maintains exclusive access to the device and provides communication with
 * userspace
 */
static long kic_device_ioctl(struct file *file, unsigned int ioctl_mode,
                             unsigned long ioctl_carry)
{
        if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
                pr_alert("KIC: Device is busy.\n");
                return -EBUSY;
        }

        pr_info("KIC: Activating IOCTL protocol.\n");
        switch(ioctl_mode) {
        case KIC_RESET_QUERY: {         // option for resetting counter
                kic_reset();
                pr_info("KIC: Counter has been reset.\n");
                break;
        }// case closed
        case KIC_GET_COUNT_QUERY: {     // option for retrieving count
                pr_info("KIC: Count retrieved at %u.\n", kic_interrupt_count);
                put_user(kic_interrupt_count, (unsigned int __user *)ioctl_carry);
                break;
        }// case closed
        case KIC_GET_TIME_QUERY: {      // option for retrieving time and date
                if (kic_time == 0) {
                        pr_alert("KIC: Timer has not yet been reset.\n");
                        put_user(-1, (long int __user *)ioctl_carry);
                        break;
                }
                put_user(kic_time, (long int __user *)ioctl_carry);
                pr_info("KIC: Time retrieved.\n");
                break;
        }// case closed
        }// switch closed
        atomic_set (&already_open, CDEV_NOT_USED);
        return 0;
}

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = kic_device_open,
        .release = kic_device_release,
        .unlocked_ioctl = kic_device_ioctl,
};

        // purpose specific functions //
// increases count upon detecting a valid interrupt
static irqreturn_t kic_increment(int irq, void *dev_id)
{
        kic_interrupt_count++;
        return IRQ_HANDLED;
}

// resets counter and saves time data when invoked
static void kic_reset(void)
{
        kic_time = ktime_get_real();
        kic_interrupt_count = 0;
}

        // init and exit //
static int __init kic_init(void)
{
        int irq_ret;

        irq_ret = register_chrdev(KIC_MAJOR, KIC_DEVICE_NAME, &fops);
        if (irq_ret < 0) {
                pr_alert("KIC: Registering device failed with error code: %d\n",
                         irq_ret);
                goto return_status;
        }
        pr_info("KIC: Device assigned major number %d\n", KIC_MAJOR);


        kic_cls = class_create(THIS_MODULE, KIC_DEVICE_NAME);
        if ((void *)kic_cls == ERR_PTR) {
                pr_alert("KIC: Class creation failed.\n");
                goto class_fail;
        }

        /*
         * may be slightly confusing, casting required for comparison,
         * pointer returned from device_crate is useful only for error handling
         */
        if ((void *)(device_create(kic_cls, NULL, MKDEV(KIC_MAJOR, 0), NULL,
            KIC_DEVICE_NAME)) == ERR_PTR) {
                pr_alert("KIC: Creating device failed with error code: %d\n",
                         irq_ret);
                goto device_fail;
        }
        pr_info("KIC: Device created on /dev/%s\n", KIC_DEVICE_NAME);


        irq_ret = request_irq(KEYBOARD_IRQ, kic_increment, KIC_IRQ_FLAGS,
                                  KIC_DEVICE_NAME, (void *)kic_cls);
        if (irq_ret < 0) {
                pr_alert("KIC: Requesting interrupt handler failed with code: %d\n",
                        irq_ret);
                goto irq_fail;
        }
        pr_info("KIC: Interrupt Handler ready.\n");

        return irq_ret;

        irq_fail:
                device_destroy(kic_cls, MKDEV(KIC_MAJOR, 0));
        device_fail:
                class_destroy(kic_cls);
        class_fail:
                unregister_chrdev(KIC_MAJOR, KIC_DEVICE_NAME);
        return_status:
                return irq_ret;
}

/*
 * waits for other irq handlers on the same line to clear
 * before exiting to avoid a possible deadlock
 */
static void __exit kic_exit(void)
{
        synchronize_irq(KEYBOARD_IRQ);

        free_irq(KEYBOARD_IRQ, (void *)kic_cls);
        device_destroy(kic_cls, MKDEV(KIC_MAJOR, 0));
        class_destroy(kic_cls);
        unregister_chrdev(KIC_MAJOR, KIC_DEVICE_NAME);
        pr_info("KIC: Removal Complete. KIC Module Unloaded\n");
}

module_init(kic_init);
module_exit(kic_exit);