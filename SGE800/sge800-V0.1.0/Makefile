CC = arm-linux-gcc
AR = arm-linux-ar

CFLAGS = -O2 -Wall -D_REENTRANT -D_GNU_SOURCE -fPIC
LDFLAGS = -lpthread 

LIB = libsge800
LIBSUB = libsge800.a 
#../ucgui/Debug/libucgui.a

OBJS = adc.o comport.o gpio.o net.o powercheck.o rtc.o timer.o thread.o msg.o
	
SRCS = ${OBJS:%.o=%.c}

all: depend $(LIB)

depend:
	$(CC) $(CFLAGS) -MM $(SRCS)  > .depend

-include .depend

$(LIB):$(OBJS)
	$(AR) -rc $@.a $(OBJS)
	$(CC) -shared -Wl,--whole-archive $(LIBSUB) -Wl,--no-whole-archive  -Wl,-soname -Wl,libsge800.so -o libsge800.so
	-cp libsge800.a ../sge800-frame/
	
clean:
	-rm -f *.o .depend $(LIB).*
#	cd ./test && $(MAKE) clean