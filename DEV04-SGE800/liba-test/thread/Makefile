CC = arm-linux-gcc
AR = arm-linux-ar

#CFLAGS = -O2 -Wall -D_REENTRANT -D_GNU_SOURCE
CFLAGS = -g -O0 -Wall -D_REENTRANT -D_GNU_SOURCE
LDFLAGS = -ldb -lpthread -L../../liba/ -lsge800

# current dir source
SRC = $(wildcard *.c)
APP = $(SRC:%.c=%) 

all  :  $(APP)
$(APP): %: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
clean:
	rm -fr $(APP)   
