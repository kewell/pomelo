CC = arm-linux-gcc
AR = arm-linux-ar

CFLAGS = -O0 -g -Wall -D_REENTRANT -D_GNU_SOURCE

SRC = $(wildcard *.c)
APP = $(SRC:%.c=%)

all  :  $(APP)
$(APP): %: %.c
	@ if (test lcd.c -nt version.h); then \
		echo " #define MODIFY_TIME \"$(shell date +%F_%T)\" " > version.h; \
	fi;
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	'cp' lcd /tftp/
	sudo 'cp' lcd /home/wenjing/WIN/G/
clean:
	rm -fr $(APP)   
