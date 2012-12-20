# Edit TCL_INCLUDE_DIR, TCL_LIBRARY_DIR, and SHAPETCL_LIB to suit your platform.

# Tcl's pkg_mkIndex will only process files named with the current platform's
# [info sharedlibextension], hence the platform-dependent SHAPETCL_LIB value.

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

# Tools
CC = /usr/bin/gcc
TCL = /usr/bin/tclsh

# Shapelib: implicit default build rules are sufficient to build needed files.
SHAPELIB_PREFIX = shapelib
SHAPELIB_OBJS = $(SHAPELIB_PREFIX)/shpopen.o $(SHAPELIB_PREFIX)/dbfopen.o $(SHAPELIB_PREFIX)/safileio.o

CFLAGS = -g -fPIC -Wall -Werror -DUSE_TCL_STUBS

.PHONY: all clean test doc

all: $(SHAPETCL_LIB)

shapetcl.o: shapetcl.c
	$(CC) $(CFLAGS) -c shapetcl.c -I$(SHAPELIB_PREFIX) -I$(TCL_INCLUDE_DIR)

$(SHAPETCL_LIB): shapetcl.o $(SHAPELIB_OBJS)
	$(CC) -o $(SHAPETCL_LIB) shapetcl.o $(SHAPELIB_OBJS) -shared -L$(TCL_LIBRARY_DIR) -ltclstub8.5
	@echo "pkg_mkIndex . " $(SHAPETCL_LIB) | $(TCL)

# clean: Remove library and object files.
clean:
	rm -f $(SHAPETCL_LIB) shapetcl.o $(SHAPELIB_OBJS)

# test: Run all scripts in the test suite.
test: $(SHAPETCL_LIB)
	$(TCL) tests/all.tcl

# doc: Convert documentation into various formats.
doc:
	cd doc && $(TCL) FormatDocs.tcl
