obj-m := atmel_adc.o
-include ../Makefile
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp atmel_adc.ko $(INSTALLDIR)
clean:
	-rm -rf *.o *.ko *.mod.c .*.cmd .tmp_versions Module.symvers modules.order

install:
	cp atmel_adc.ko $(INSTALLDIR)
uninstall:
	-rm  $(INSTALLDIR)/atmel_adc.ko 
