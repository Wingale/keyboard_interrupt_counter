obj-m += kic_module.o

KVERSION := $(shell uname -r)
PWD := $(CURDIR)

all: keyboard_interrupt_counter.h
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	sudo rmmod kic_module
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

enable:
	sudo dmesg -C
	sudo insmod kic_module.ko
	gcc -o kic kic_user.c