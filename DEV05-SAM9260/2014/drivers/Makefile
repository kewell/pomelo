obj-m := L350BeepDrv.o
obj-m += L350LedDrv.o
obj-m += L350ModemDrv.o
obj-m += L350Rs485Drv.o
obj-m += L350GTM900Drv.o
obj-m += uc864e_usb.o

ARCH = arm
KERNEL_VER := linux-2.6.22
CROSS_COMPILE := arm-linux-
PWD := $(shell pwd)

LINUX_SRC := /root/$(KERNEL_VER)/
MODULES_PATH := ./$(KERNEL_VER)/

all: howto clean modules

howto:
	@echo  ' '
	@echo  '		########################################################'
	@echo  '		#         Usage:  make LINUX_VER=LINUX-2.6.33          #'
	@echo  '		#             or  make LINUX_VER=LINUX-2.6.22          #'      
	@echo  '		########################################################'
	@echo  ' '

modules:
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) modules

install:
	@'cp'  -f *.ko  $(MODULES_PATH)

tag:
	@ctags --c-kinds=+defglmnstuvx --langmap=c:.c.h.ho.hem.het.hec.hev.him.hit.hic.hiv -R .
	@cscope -Rbq

clean:
	@rm -rf *.o *.ko *~ core .depend .*.cmd  *.mod.c .tmp_versions Module.symvers -f
	@rm -f cscope* tags
