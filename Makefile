# your favourite C compiler here
CC=gcc

VERSION=0.4

TABLE_OBJS=	Table_Init.o Table_Event.o Table_Cmd.o \
		Table_Display.o Table_Config.o

# set up the following variables to point to the the tcl and tk directories.
PREFIX= /usr/add-on/tcl7.5
INCDIR=	$(PREFIX)/include
LIBDIR=	$(PREFIX)/lib
BINDIR=	$(PREFIX)/bin
MANDIR=	$(PREFIX)/man

# SunOS 4.1.x
#SHLIB_LD =     ld -assert pure-text
#SYSLIBS = -lm
#X11INC=	/usr/include/X11
#X11LIB=	-L/usr/lib/X11 -lX11
#RL_LIBDIR=-L$(LIBDIR)

# Solaris 2.x
SHLIB_LD =      ld -G -ztext
SYSLIBS = -lsocket -lnsl -lm
X11INC=	/opt/X11R6/include
X11LIB=	-R/opt/X11R6/lib -L/opt/X11R6/lib -lX11
RL_LIBDIR=-R$(LIBDIR) -L$(LIBDIR)

# Any compilation flags that you think you need go here.
FLAGS=	-fpic -O -g

CFLAGS= $(FLAGS) -I$(INCDIR) -I$(X11INC)


all:	tablewish

# this builds a wish shell with the Table widget compiled in.
tablewish:	Makefile version.h $(TABLE_OBJS) tkAppInit.o
	$(CC) -o tablewish tkAppInit.o $(TABLE_OBJS) \
		$(RL_LIBDIR) -ltk4.1 -ltcl7.5 $(X11LIB) $(SYSLIBS)

libtktable.a: Makefile version.h $(TABLE_OBJS)
	ar r libtktable.a $(TABLE_OBJS)
	ranlib libtktable.a

version.h: Makefile
	echo "#define TKTABLE_VERSION \"$(VERSION)\"" >version.h

install: Makefile version.h $(TABLE_OBJS) tkAppInit.o
	rm -f $(LIBDIR)/libtktable.so.$(VERSION)
	$(SHLIB_LD) -o $(LIBDIR)/libtktable.so.$(VERSION) $(TABLE_OBJS)
	rm -f $(LIBDIR)/libtktable.so
	ln -s $(LIBDIR)/libtktable.so.$(VERSION) $(LIBDIR)/libtktable.so
	$(CC) -o $(BINDIR)/tablewish tkAppInit.o \
		$(RL_LIBDIR) -ltktable -ltk4.1 -ltcl7.5 $(X11LIB) $(SYSLIBS)
	cp table.n $(MANDIR)/mann/

clean:
	rm -f *.[ao] tablewish core
