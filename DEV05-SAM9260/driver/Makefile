ARCH = AT91SAM9260
PLAT ?= SAM9260
HWVER ?= OLD
#HWVER ?= NEW

#KERNEL_VER = linux-2.6.22.1
KERNEL_VER = linux-3.6-rc4

LINUX_SRC ?= ../$(KERNEL_VER)
CROSS_COMPILE ?= /opt/buildroot-2012.08/output/host/usr/bin/arm-linux-

PWD := $(shell pwd)

EXTRA_CFLAGS+=-DPLAT_${PLAT}
EXTRA_CFLAGS+=-DARCH_${ARCH}
EXTRA_CFLAGS+=-DHWVER_${HWVER}
EXTRA_CFLAGS+=-D__KERNEL__

obj-m := dev_led.o
obj-m += dev_beep.o
obj-m += dev_detect.o
obj-m += dev_gprs.o
dev_gprs-objs  :=  dev_gprs_main.o gprs_lowlevel.o

modules:
	@make -C $(LINUX_SRC) M=$(PWD) modules
	@make clear

tag:
	@ctags --c-kinds=+defglmnstuvx --langmap=c:.c.h.ho.hem.het.hec.hev.him.hit.hic.hiv -R .
	@cscope -Rbq

clear:
	@rm -f *.o *.cmd *.mod.c
	@rm -rf  *~ core .depend  .tmp_versions Module.symvers modules.order -f
	@rm -f .*ko.cmd .*.o.cmd

clean: clear
	@rm -f  *.ko

distclean:  clear
	@rm -f cscope* tags
	@rm -f *.ko

