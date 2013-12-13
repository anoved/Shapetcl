# Redefine these environment variables as needed to select appropriate Tcl.
ifndef TCL_INCLUDE_DIR
TCL_INCLUDE_DIR = /usr/include/tcl8.5
endif
ifndef TCL_LIBRARY_DIR
TCL_LIBRARY_DIR = /usr/lib
endif

# Shapelib: implicit default build rules are sufficient to build needed files.
SHAPELIB_DIR = shapelib
SHAPELIB_OBJS = $(SHAPELIB_DIR)/shpopen.o $(SHAPELIB_DIR)/dbfopen.o $(SHAPELIB_DIR)/safileio.o

CC = /usr/bin/gcc
TCL = /usr/bin/tclsh
CFLAGS = -g -fPIC -Wall -Werror -DUSE_TCL_STUBS

.PHONY: all install clean test doc

all: shapetcl.so

# shapetcl.o: Compile the Shapetcl code.
shapetcl.o: shapetcl.c
	$(CC) $(CFLAGS) -c shapetcl.c -I$(SHAPELIB_DIR) -I$(TCL_INCLUDE_DIR)

# shapetcl.so: Link the Shapetcl and Shapelib object code into a shared library.
shapetcl.so: shapetcl.o $(SHAPELIB_OBJS)
	$(CC) -o shapetcl.so shapetcl.o $(SHAPELIB_OBJS) -shared -L$(TCL_LIBRARY_DIR) -ltclstub8.5

# install: Put the shared library somewhere in the auto_path (possibly system/sudo dependent)
install: shapetcl.so
	mkdir -p /usr/local/lib/tcltk
	cp pkgIndex.tcl shapetcl.so /usr/local/lib/tcltk

# clean: Remove shared library and compiled object files.
clean:
	rm -f shapetcl.so shapetcl.o $(SHAPELIB_OBJS)

# test: Run all scripts in the test suite.
test: shapetcl.so
	$(TCL) tests/all.tcl

# analyze: run static analysis tests
analyze: shapetcl.so
	$(TCL) tests/static.test.tcl -constraint static

# doc: Convert documentation into various formats.
doc:
	cd doc && $(TCL) FormatDocs.tcl
