#include<linux/ioctl.h>

#define KIC_DEVICE_NAME "keyboard_interrupt_counter"

#define KIC_MAJOR 420

#define KIC_DEVICE_PATH "/dev/keyboard_interrupt_counter"

#define KIC_GET_COUNT_QUERY _IOR(KIC_MAJOR, 0, unsigned int *)
#define KIC_GET_TIME_QUERY _IOR(KIC_MAJOR, 1, long int *)
#define KIC_RESET_QUERY _IO(KIC_MAJOR, 2)

#define KEYBOARD_IRQ 1
#define KIC_IRQ_FLAGS IRQF_SHARED