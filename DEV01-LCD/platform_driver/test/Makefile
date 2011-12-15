#*********************************************************************************
#      Copyright:  (C) 2011 China Undergraduate Embedded League(CUEL)
#                  All rights reserved.
#
#       Filename:  Makefile
#    Description:  This Makefile used to compile all the C source code file in current 
#                  folder to respective excutable binary files.
#                      
#        Version:  1.0.0(10/08/2011~)
#                  Author:  Guo Wenxue <guowenxue@gmail.com>
#      ChangeLog:  1, Release initial version on "11/11/2011 01:29:33 PM"
#                       
#********************************************************************************/
INSTALL_DIR=~/tftp

#=========CFLAGS & LDFLAGS ================
LINK_MODE=STATIC
MODE=PRODUCTION
DEBUG=1

#LDFLAGS+=-lpthread
CFLAGS+=-Wall -Werror

ifeq ("${MODE}", "PRODUCTION")
    CFLAGS+=-DPRODUCTION_MODE
endif

ifdef DEBUG
    CFLAGS+=-g -DDEBUG
endif

ifeq ("${LINK_MODE}", "STATIC")
	CFLAGS+=--static
	LDFLAGS+=-static
else
	LDFLAGS+=-ldl
endif


#============ Define gcc========================
gcc_pre ?=

CC=$(gcc_pre)gcc
CXX=$(gcc_pre)gcc
AR=$(gcc_pre)ar
AS=$(gcc_pre)as
RANLIB=$(gcc_pre)ranlib
STRIP=$(gcc_pre)strip


#============Compile=====================
SRC_FILE=$(wildcard *.c)
BIN_FILE=$(SRC_FILE:.c=)
DEP_FILE=$(SRC_FILE:.c=.d)

all:  $(BIN_FILE) $(DEP_FILE) 

.PHONY: all clean 

			
-include $(DEP_FILE)

%.d: %.c
	@svnrev *; \
	set -e; rm -f $@; \
	$(CC) -MM $(<) > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1 : ,g' < $@.$$$$ > $@; \
	#sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

print:

install:
	cp $(BIN_FILE) $(INSTALL_DIR)
clean:
	$(RM) *.d *.d.* *.o $(BIN_FILE) svnrev.h
	



