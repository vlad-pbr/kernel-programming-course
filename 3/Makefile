obj-m += km.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

insert:
	sudo insmod km.ko

remove:
	-sudo rmmod km

reinsert: remove clean all insert
	
run:
	rm -f a.out && gcc proc.c && ./a.out $(args)

log:
	sudo dmesg