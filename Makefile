# Path to Shapelib distribution directory.
# Tested with Shapelib 1.3.0, made with no modifications.
SHAPELIB_PREFIX = ../shapelib-1.3.0

# Path to directory containing /include and /lib subdirectories
# containing Tcl headers and shared library, respectively.
TCL_PREFIX = /usr

SHAPELIB_OBJS = $(SHAPELIB_PREFIX)/shpopen.o \
				$(SHAPELIB_PREFIX)/dbfopen.o \
				$(SHAPELIB_PREFIX)/safileio.o

CFLAGS = -g -fPIC -Wall -Werror
CC = gcc
TCL = tclsh

.PHONY: all clean test

all: Shapetcl.dylib

shapetcl.o: shapetcl.c
	$(CC) $(CFLAGS) -c shapetcl.c \
			-I$(SHAPELIB_PREFIX) \
			-I$(TCL_PREFIX)/include

Shapetcl.dylib: shapetcl.o $(SHAPELIB_OBJS)
	$(CC) -shared -W1,-soname,Shapetcl \
			-o Shapetcl.dylib \
			shapetcl.o $(SHAPELIB_OBJS) \
			-L$(TCL_PREFIX)/lib -ltcl8.5
	echo "pkg_mkIndex . shapetcl.dylib" | $(TCL)

clean:
	rm -f Shapetcl.dylib shapetcl.o

test:
	@echo "No tests defined."
