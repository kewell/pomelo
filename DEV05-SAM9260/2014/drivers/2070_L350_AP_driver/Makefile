#MODE  STA or AP or APSTA
RT28xx_MODE = AP
TARGET = LINUX
CHIPSET = 2070
RT28xx_DIR = $(shell pwd)
RTMP_SRC_DIR = $(RT28xx_DIR)/RT$(CHIPSET)
PLATFORM = L350
RELEASE = DPA

ifeq ($(PLATFORM),L350)
LINUX_SRC = /usr/Devices/L350/linux-2.6.22.1/
LINUX_SRC = /usr/Devices/L350/linux-2.6.33.7/
LINUX_SRC = /root/9260/linux-3.6-rc4/
LINUX_SRC = /root/linux-2.6.22/
CROSS_COMPILE = arm-linux-
endif

export RT28xx_DIR RT28xx_MODE LINUX_SRC CROSS_COMPILE CROSS_COMPILE_INCLUDE PLATFORM RELEASE CHIPSET RTMP_SRC_DIR LINUX_SRC_MODULE

all: build_tools $(TARGET)


build_tools:
	make -C tools
	$(RT28xx_DIR)/tools/bin2h

UCOS:
	make -C os/ucos/ MODE=$(RT28xx_MODE)
	echo $(RT28xx_MODE)


LINUX:
ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	cp -f os/linux/Makefile.4 $(RT28xx_DIR)/os/linux/Makefile
	make -C $(RT28xx_DIR)/os/linux/
ifeq ($(RT28xx_MODE),AP)
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)ap.o /tftpboot
ifeq ($(PLATFORM),INF_AMAZON_SE)
	cp -f /tftpboot/rt2870ap.o /backup/ifx/build/root_filesystem/lib/modules/2.4.31-Amazon_SE-3.6.2.2-R0416_Ralink/kernel/drivers/net
endif
ifeq ($(PLATFORM),INF_AMAZON_SE_THOMSON)
        cp -f /tftpboot/rt3070ap.ko /backup/platforms/ASEBSP1.1.0.1/build/root_filesystem/lib/modules
endif
else	
ifeq ($(RT28xx_MODE),APSTA)
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)apsta.o /tftpboot
else
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)sta.o /tftpboot
endif	
endif	
else
	cp -f os/linux/Makefile.6 $(RT28xx_DIR)/os/linux/Makefile
	make  -C  $(LINUX_SRC) SUBDIRS=$(RT28xx_DIR)/os/linux modules
ifeq ($(RT28xx_MODE),AP)
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)ap.ko ../../../rootfs/L350_rootfs/usr/drivers/
#	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)ap.ko $(RT28xx_DIR)/../Wireless/rt2070ap/
#	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)ap.ko /usr/local/src/image
ifeq ($(PLATFORM),INF_AMAZON_SE_THOMSON)
	cp -f /tftpboot/rt3070ap.ko /backup/platforms/ASEBSP1.1.0.1/build/root_filesystem/lib/modules
endif

else	
ifeq ($(RT28xx_MODE),APSTA)
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)apsta.ko /tftpboot
else
	cp -f $(RT28xx_DIR)/os/linux/rt$(CHIPSET)sta.ko /tftpboot
endif	
endif
endif

release:
ifeq ($(TARGET), LINUX)
	make -C $(RT28xx_DIR)/os/linux -f Makefile.release release
ifeq ($(RELEASE), DPA)
	make -C $(RT28xx_DIR)/os/linux -f Makefile.DPA release
	rm -rf build
endif
ifeq ($(RELEASE), DPB)
	make -C $(RT28xx_DIR)/os/linux -f Makefile.DPB release
	rm -rf build
endif
ifeq ($(RELEASE), DPC)
	make -C $(RT28xx_DIR)/os/linux -f Makefile.DPC release
	rm -rf build
endif
ifeq ($(RT28xx_MODE),STA)
ifeq ($(RELEASE), DPD)
	make -C $(RT28xx_DIR)/os/linux -f Makefile.DPD release
	rm -rf build
endif

ifeq ($(RELEASE), DPO)
	make -C $(RT28xx_DIR)/os/linux -f Makefile.DPO release
	rm -rf build
endif
endif
endif

prerelease:
	make -C $(RT28xx_DIR)/os/linux -f Makefile.release prerelease
	cp $(RT28xx_DIR)/os/linux/Makefile.DPB $(RTMP_SRC_DIR)/os/linux/.
	cp $(RT28xx_DIR)/os/linux/Makefile.DPA $(RTMP_SRC_DIR)/os/linux/.
	cp $(RT28xx_DIR)/os/linux/Makefile.DPC $(RTMP_SRC_DIR)/os/linux/.
ifeq ($(RT28xx_MODE),STA)
	cp $(RT28xx_DIR)/os/linux/Makefile.DPD $(RTMP_SRC_DIR)/os/linux/.
	cp $(RT28xx_DIR)/os/linux/Makefile.DPO $(RTMP_SRC_DIR)/os/linux/.
endif	

clean:
ifeq ($(TARGET), LINUX)
ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	cp -f os/linux/Makefile.4 os/linux/Makefile
else
	cp -f os/linux/Makefile.6 os/linux/Makefile
endif
	make -C os/linux clean
	rm -rf os/linux/Makefile
endif	
ifeq ($(TARGET), UCOS)
	make -C os/ucos clean MODE=$(RT28xx_MODE)
endif

uninstall:
ifeq ($(TARGET), LINUX)
ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	make -C $(RT28xx_DIR)/os/linux -f Makefile.4 uninstall
else
	make -C $(RT28xx_DIR)/os/linux -f Makefile.6 uninstall
endif
endif

install:
ifeq ($(TARGET), LINUX)
ifneq (,$(findstring 2.4,$(LINUX_SRC)))
	make -C $(RT28xx_DIR)/os/linux -f Makefile.4 install
else
	make -C $(RT28xx_DIR)/os/linux -f Makefile.6 install
endif
endif
