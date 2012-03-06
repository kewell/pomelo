obj-m := atmel_tc.o
-include ../Makefile
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp atmel_tc.ko $(INSTALLDIR)
clean:
	-rm -rf *.o *.ko *.mod.c .*.cmd .tmp_versions Module.symvers modules.order
install:
	cp atmel_gpio.ko $(INSTALLDIR)
uninstall:
	-rm -f $(INSTALLDIR)/atmel_tc.ko 
	