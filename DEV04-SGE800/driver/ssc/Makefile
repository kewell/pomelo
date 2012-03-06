obj-m += uda1330.o
-include ../Makefile
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules	
	cp uda1330.ko $(INSTALLDIR)
clean:
	-rm -rf *.o *.ko *.mod.c .*.cmd .tmp_versions Module.symvers modules.order

# test driver
debug:
#	rm -r ssc $(INSTALLDIR)/ssc
	arm-linux-gcc  sound.c -o sound
	arm-linux-gcc  mixer.c -o mixer
	cp sound $(INSTALLDIR)
	cp mixer $(INSTALLDIR)
install:
	cp uda1330.ko $(INSTALLDIR)
uninstall:
	-rm  $(INSTALLDIR)/uda1330.ko $(INSTALLDIR)/sound $(INSTALLDIR)/mixer