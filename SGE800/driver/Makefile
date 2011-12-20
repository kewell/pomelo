CC = arm-linux-gcc
AR = arm-linux-ar

KERNELDIR := /home/roy/kernel/linux-2.6.33
INSTALLDIR := /home/roy/work/win
PWD := $(shell pwd)

drive:ad adi gpio ps rtc  tc powerd 
.PHONY:ad adi fish gpio ps rtc  tc powerd
ad:
	cd ./ad && $(MAKE) all	
adi:
	cd ./adi && $(MAKE) all	
fish:
	cd ./fish && $(MAKE) all
gpio:
	cd ./gpio && $(MAKE) all
ps:
	cd ./ps && $(MAKE) all
rtc:
	cd ./rtc && $(MAKE) all
ssc:
	cd ./ssc && $(MAKE) all
tc:
	cd ./tc && $(MAKE) all
powerd:
	cd ./powerdeteck && $(MAKE) all

distclean:
	cd ./ad && $(MAKE) clean
	cd ./adi && $(MAKE) clean	
	cd ./fish && $(MAKE) clean
	cd ./gpio && $(MAKE) clean
	cd ./ps && $(MAKE) clean
	cd ./rtc && $(MAKE) clean
	cd ./ssc && $(MAKE) clean
	cd ./tc && $(MAKE) clean
	cd ./powerdeteck && $(MAKE) clean