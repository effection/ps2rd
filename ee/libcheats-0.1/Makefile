##
# Makefile to build libcheats for PS2
#
# Copyright (C) 2009 misfire <misfire@xploderfreax.de>
##

EE_INCS += -I ./include
EE_CFLAGS +=
EE_LDFLAGS +=
EE_ASFLAGS +=

EE_LIB = lib/libcheats.a
EE_OBJS = src/cheatlist.o src/libcheats.o src/mystring.o src/parser.o
EE_LIBS =


all: $(EE_LIB)

install: all
	mkdir -p $(PS2SDK)/ports/include
	mkdir -p $(PS2SDK)/ports/lib
	cp -f $(EE_LIB) $(PS2SDK)/ports/lib
	cp -f include/*.h $(PS2SDK)/ports/include

clean:
	rm -f $(EE_LIB) $(EE_OBJS)


include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
