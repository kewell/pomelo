obj-m := atmel_gpio.o

-include ../Makefile
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules 
	cp atmel_gpio.ko $(INSTALLDIR)
clean:
	-rm -rf *.o *.ko *.mod.c .*.cmd .tmp_versions Module.symvers modules.order

install:
	cp atmel_gpio.ko $(INSTALLDIR)
uninstall:
	-rm -f $(INSTALLDIR)/atmel_gpio.ko 