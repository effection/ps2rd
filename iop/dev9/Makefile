IOP_BIN  = ps2dev9.irx

IOP_INCS +=
IOP_CFLAGS  += -Wall -fno-builtin-printf -DDISBABLE_DEV9SHUTDOWN -DDEV9X_DEV #-DCHECK_LOADED -DPOWEROFF -DPCMCIA -DDEBUG
IOP_LDFLAGS += -s

IOP_OBJS = ps2dev9.o exports.o imports.o
IOP_LIBS =

all: $(IOP_BIN)

clean:
	rm -f $(IOP_BIN) $(IOP_OBJS)

run:
	ps2client -t 1 execiop host:$(IOP_BIN)


include $(PS2SDK)/Defs.make
include Rules.make
