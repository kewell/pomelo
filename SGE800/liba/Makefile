CC = arm-linux-gcc
AR = arm-linux-ar

CFLAGS = -O0 -g -Wall -D_REENTRANT -D_GNU_SOURCE -fPIC
LDFLAGS =  -lpthread 

LIB = libsge800
LIBSUB = libsge800.a 
#../ucgui/Debug/libucgui.a

OBJS = adc.o timer.o rtc.o gpio.o powercheck.o net.o comport.o file.o thread.o msg.o syn.o \
	relay.o led.o swin.o key.o dcsig.o cyclicdb.o gprs_me3000.o sdev_485.o sdev_net.o sdev.o
SRCS = ${OBJS:%.o=%.c}

all: depend $(LIB)

depend:
	$(CC) $(CFLAGS) -MM $(SRCS)  > .depend

-include .depend

$(LIB):$(OBJS)
	$(AR) -rc $@.a $(OBJS)
#	$(CC) -shared -Wl,--whole-archive $(LIBSUB) -Wl,--no-whole-archive  -Wl,-soname -Wl,libsge800.so -o libsge800.so
#	-cp libsge800.so ../app/Debug
	
clean:
	-rm -f *.o .depend $(LIB).*
	cd ./test && $(MAKE) clean