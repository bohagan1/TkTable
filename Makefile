
# your favourite C compiler here
CC=/usr/local/lang/acc

VERS=0.1

TABLE_OBJS=	Table_Init.o Table_Event.o Table_Cmd.o\
		Table_Display.o Table_Config.o

# Any compilation flags that you think you need go here
# Debugabble version:
#FLAGS=	-g 
FLAGS=

# set up the following variables to point to the the tcl and
# tk directories. If you are using extended tcl and tk
# you wiill need to add those directories too
TKDIR=	/home/tkdevel/Tcl/tk3.6
TCLDIR=	/home/tkdevel/Tcl/tcl7.3

# the X11 directory path
X11=	/usr/X11R5

CFLAGS= $(FLAGS) -I/usr/local/include -I$(TKDIR) -I$(TCLDIR) -I$(X11)/include

all:	Table.so.$(VERS) wish


# this builds the shared library version for Sun 4.1.3
Table.so.$(VERS): $(TABLE_OBJS) 
	ld -o Table.so.$(VERS) $(TABLE_OBJS) 


# this builds a wish shell with the Table widget compiled in. 
wish:	Makefile $(TABLE_OBJS) tkAppInit.o
	$(CC) tkAppInit.o $(TABLE_OBJS)\
		-L$(TCLDIR) -L$(TKDIR)\
		-ltk -ltcl -lXpm -lX11 -lm -o wish

clean:
	rm -f *.o
