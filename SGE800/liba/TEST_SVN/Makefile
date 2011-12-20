CC = arm-linux-gcc
AR = arm-linux-ar

CFLAGS = -O0 -g -Wall -D_REENTRANT -D_GNU_SOURCE 

LDFLAGS =   -lpthread -L../  -lsge800
	
SRC = $(wildcard *.c)
APP = $(SRC:%.c=%) 

all  :  $(APP)
$(APP): %: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
clean:
	rm -fr $(APP)   