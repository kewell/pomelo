obj-m := mcu.o
-include ../Makefile
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp mcu.ko $(INSTALLDIR)
clean:
	-rm -rf *.o *.ko *.mod.c .*.cmd .tmp_versions Module.symvers modules.order

install:
	cp mcu.ko $(INSTALLDIR)
uninstall:
	-rm  $(INSTALLDIR)/mcu.ko 
