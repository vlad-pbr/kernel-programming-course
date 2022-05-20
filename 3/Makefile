obj-m += km.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

insert:
	sudo insmod km.ko
	sudo mknod /dev/km c $(shell sudo dmesg | grep "device major number" | tail -n 1 | cut -d' ' -f 9) 0

remove:
	sudo rm -f /dev/km
	sudo rmmod km

log:
	sudo dmesg
	