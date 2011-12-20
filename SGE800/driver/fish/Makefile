obj-m := fishb.o

-include ../Makefile

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	cp fishb.ko $(INSTALLDIR)
clean:
	-rm -rf *.o *.ko *.mod.c .*.cmd .tmp_versions Module.symvers modules.order

# test driver
debug:
#	rm -r tlv $(INSTALLDIR)/tlv
	$(CC)  fb.c -o fb
	cp fb $(INSTALLDIR)
install:
	cp fishb.ko $(INSTALLDIR)
