obj-m += km.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

insert:
	sudo insmod km.ko

remove:
	sudo rmmod km

log:
	sudo dmesg
	