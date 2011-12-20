obj-m := atmel_powerd.o
-include ../Makefile
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp atmel_powerd.ko $(INSTALLDIR)
clean:
	-rm -rf *.o *.ko *.mod.c .*.cmd .tmp_versions Module.symvers modules.order

install:
	cp atmel_powerd.ko $(INSTALLDIR)
uninstall:
	-rm -f $(INSTALLDIR)/atmel_powerd.ko 