CC ?= gcc

sqlite3 = 1

# check the release/debug flags
ifeq ($(release), 1)
	CFLAGS := -O2
else
	CFLAGS := -g -Wall -W -Wextra -Wshadow -Wconversion  -Wwrite-strings  -Wcast-qual
endif

INCLUDE :=$(shell pkg-config --cflags  glib-2.0)
LIBFLAGS := $(shell pkg-config --libs  glib-2.0)


CFILES := keyfile.c collect.c communicate.c config.c function.c log.c
OBJS := $(subst .c,.o,$(CFILES))
HAST3_BIN := hast3
EXE := $(HAST3_BIN)

# set the build time
DATE := $(shell date +%F)
CFLAGS += -D__BUILD_DATE__="\"$(DATE)\""

# set the arch
ARCH := $(shell uname -m)
CFLAGS += -D__BUILD_ARCH__="\"$(ARCH)\""

# check if verbose
ifeq ($(verbose), 1)
	VERBOSE :=
else
	VERBOSE := @
endif

ALL:	$(HAST3_BIN) receiver

$(HAST3_BIN):	$(OBJS)	main.c
	$(VERBOSE)$(CC) $(CFLAGS) $(INCLUDE) main.c $(OBJS) $(LIBFLAGS) -o $(HAST3_BIN) 

$(OBJS): %.o : %.c
	$(VERBOSE)$(CC) $(CFLAGS) $(INCLUDE) -c $<  

.PHONY : install clean tags distclean ALL

install:
	$(VERBOSE)cp -f $(AST3D) $(AST3D_TOOL) ../bin/


clean:
	$(VERBOSE)rm -rf $(EXE) $(OBJS) receiver main.o

tags: *.c *.h	
	ctags -R *
	cscope -Rbq


distclean: clean
	rm -f cscope.* tags

receiver: receiver.c
	gcc receiver.c -o receiver
