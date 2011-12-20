obj-m := tlv1504.o
-include ../Makefile
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp tlv1504.ko $(INSTALLDIR)
clean:
	-rm -rf *.o *.ko *.mod.c .*.cmd .tmp_versions Module.symvers modules.order

install:
	cp tlv1504.ko $(INSTALLDIR)
uninstall:
	-rm  $(INSTALLDIR)/tlv1504.ko 