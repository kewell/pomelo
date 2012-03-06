obj-m := ds3231.o
-include ../Makefile
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp ds3231.ko $(INSTALLDIR)
clean:
	-rm -rf *.o *.ko *.mod.c .*.cmd .tmp_versions Module.symvers modules.order

install:
	cp ds3231.ko $(INSTALLDIR)
uninstall:
	-rm -f $(INSTALLDIR)/ds3231.ko 