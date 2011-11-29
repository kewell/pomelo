PLAT ?= CONCENTRATOR
ARCH = sam9260
KERNEL_VER = linux-2.6.35
KERNEL_VER = linux-3.0
LINUX_SRC ?= /home/wenjing/src/kernel/$(KERNEL_VER)
INST_PATH=../rootfs/mnt/usr/drivers/
INST_PATH=/tftp
HWVER ?= V1

CROSS_COMPILE ?= /opt/buildroot-2011.02/arm926t/usr/bin/arm-linux-


PWD := $(shell pwd)
TARGET = dev_lcd.ko

EXTRA_CFLAGS+=-DPLAT_${PLAT}
EXTRA_CFLAGS+=-DARCH_${ARCH}
EXTRA_CFLAGS+=-DHWVER_${HWVER}
EXTRA_CFLAGS+=-D__KERNEL__

obj-m := dev_lcd.o
#obj-m += dev_led.o
#obj-m += dev_beep.o
#obj-m += dev_faketty.o
#obj-m += dev_eeprom.o
#obj-m += dev_rtc.o
#obj-m += dev_sam.o
#obj-m += dev_key.o
#obj-m += dev_gprs.o
#dev_gprs-objs  :=  dev_gprs_main.o gprs_lowlevel.o

modules:
	@ if (test dev_lcd.c -nt include/version.h); then \
		echo " #define MODIFY_TIME \"$(shell date +%F_%T)\" " > include/version.h; \
	fi;
	@make -C $(LINUX_SRC) M=$(PWD) modules
	@make install
	@make clear

uninstall:
	rm -f $(TARGET) ${INST_PATH}/$(TARGET)

install:
	rm -rf ${INST_PATH}/$(TARGET)
	'cp' -af $(TARGET) ${INST_PATH}/$(TARGET)
	rm -rf $(TARGET)

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

