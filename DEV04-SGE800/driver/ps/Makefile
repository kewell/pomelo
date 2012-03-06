obj-m := atmel_ps.o
-include ../Makefile
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp atmel_ps.ko $(INSTALLDIR)
clean:
	-rm -rf *.o *.ko *.mod.c .*.cmd .tmp_versions Module.symvers modules.order

install:
	cp atmel_ps.ko $(INSTALLDIR)
uninstall:
	-rm -f $(INSTALLDIR)/atmel_ps.ko 