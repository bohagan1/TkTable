# your favourite C compiler here
CC=gcc

VERS=0.4

TABLE_OBJS=	Table_Init.o Table_Event.o Table_Cmd.o \
		Table_Display.o Table_Config.o

# Any compilation flags that you think you need go here.
FLAGS=	-fpic -O

# set up the following variables to point to the the tcl and tk directories.
PREFIX= /usr/add-on/tcl7.5
INCDIR=	$(PREFIX)/include
LIBDIR=	$(PREFIX)/lib
BINDIR=	$(PREFIX)/bin
MANDIR=	$(PREFIX)/man

# the X11 directory paths
X11INC=	/usr/include/X11
X11LIB=	/usr/lib/X11

CFLAGS= $(FLAGS) -I$(INCDIR) -I$(X11INC)


all:	tablewish

# this builds a wish shell with the Table widget compiled in.
tablewish:	Makefile $(TABLE_OBJS) tkAppInit.o
	$(CC) -o tablewish tkAppInit.o $(TABLE_OBJS) \
		-L$(LIBDIR) -ltk -ltcl -L$(X11LIB) -lX11 -lm

# this builds the shared library version for Sun 4.1.3
libtktable.so.$(VERS): $(TABLE_OBJS)
	ld -assert pure-text -o libtktable.so.$(VERS) $(TABLE_OBJS)

libtktable.a: $(TABLE_OBJS)
	ar r libtktable.a $(TABLE_OBJS)
	ranlib libtktable.a

install:	Makefile $(TABLE_OBJS) tkAppInit.o
	rm -f $(LIBDIR)/libtktable.so.$(VERS)
	ld -assert pure-text -o $(LIBDIR)/libtktable.so.$(VERS) $(TABLE_OBJS)
	$(CC) -o $(BINDIR)/tablewish tkAppInit.o \
		-L$(LIBDIR) -ltktable -ltk -ltcl -L$(X11LIB) -lX11 -lm
	cp table.n $(MANDIR)/mann/

clean:
	rm -f *.[ao] tablewish core
