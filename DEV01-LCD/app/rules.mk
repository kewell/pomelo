#rules.mk

#USE_PC=1

ifdef USE_PC
CC = gcc
AR = ar
LD = ld
else
CC = arm-linux-gcc
AR = arm-linux-ar
LD = arm-linux-ld
endif

MAKE = make

CFLAGS = -Wall -O2 -I$(WORK_DIR)
DFLAGS = 

DEPS := $(patsubst %.o, %.c.dep, $(OBJS))
TEMPDEPS := $(addsuffix ., $(dir $(DEPS)))
DEPS := $(join $(TEMPDEPS), $(notdir $(DEPS)))

IGNORE=$(wildcard .*.c.dep)
-include $(IGNORE)

%.o: %.c
	@echo "  CC  " $< "-o" $@; \
	$(CC) $(CFLAGS) $(DFLAGS) -c $< -o $@ -Wp,-MMD,$(dir $<).$(notdir $<).dep

