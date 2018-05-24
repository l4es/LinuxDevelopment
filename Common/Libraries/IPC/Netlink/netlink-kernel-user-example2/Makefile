obj-m += netlinksock.o
netlinksock-objs := kenrelNetlinkPSock.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean




