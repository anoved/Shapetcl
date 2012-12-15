# Path to Shapelib distribution directory.
# Tested with Shapelib 1.3.0, made with no modifications.
SHAPELIB_PREFIX = shapelib

ifeq ($(TARGET), linux)
	# invoked with eg "make TARGET=linux"
	SHAPETCL_LIB = Shapetcl.so
	TCL_INCLUDE_DIR = /usr/include/tcl8.5
	TCL_LIBRARY_DIR = /usr/lib
else
	# default values (Mac OS X)
	SHAPETCL_LIB = Shapetcl.dylib
	TCL_INCLUDE_DIR = /usr/include
	TCL_LIBRARY_DIR = /usr/lib
endif

# default build rules are sufficient to build these
SHAPELIB_OBJS = $(SHAPELIB_PREFIX)/shpopen.o \
				$(SHAPELIB_PREFIX)/dbfopen.o \
				$(SHAPELIB_PREFIX)/safileio.o

CFLAGS = -g -fPIC -Wall -Werror
CC = gcc
TCL = tclsh

.PHONY: all clean test doc

all: $(SHAPETCL_LIB)

shapetcl.o: shapetcl.c
	$(CC) $(CFLAGS) -c shapetcl.c \
			-I$(SHAPELIB_PREFIX) \
			-I$(TCL_INCLUDE_DIR)

$(SHAPETCL_LIB): shapetcl.o $(SHAPELIB_OBJS)
	$(CC) -shared \
			-o $(SHAPETCL_LIB) \
			shapetcl.o $(SHAPELIB_OBJS) \
			-L$(TCL_LIBRARY_DIR) -ltcl8.5
	echo "pkg_mkIndex . " $(SHAPETCL_LIB) | $(TCL)

clean:
	rm -f $(SHAPETCL_LIB) shapetcl.o $(SHAPELIB_OBJS)

test: $(SHAPETCL_LIB)
	$(TCL) tests/all.tcl

doc: doc/shapetcl.html

doc/shapetcl.html: doc/shapetcl.dtp
	doc/dt.tcl <doc/shapetcl.dtp >>doc/shapetcl.html
