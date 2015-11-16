obj-m += nodeadlock_module.o
KDIR= /lib/modules/3.13.0-62-generic/build
all: 
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules 
clean: 
	rm -rf *.o *.ko *.mod.* .c* .t*
