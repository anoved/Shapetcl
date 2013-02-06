# Redefine these environment variables as needed to select appropriate Tcl.
ifndef TCL_INCLUDE_DIR
TCL_INCLUDE_DIR = /usr/include
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

.PHONY: all clean test doc

all: shapetcl.so

# shapetcl.o: Compile the Shapetcl code.
shapetcl.o: shapetcl.c
	$(CC) $(CFLAGS) -c shapetcl.c -I$(SHAPELIB_DIR) -I$(TCL_INCLUDE_DIR)

# shapetcl.so: Link the Shapetcl and Shapelib object code into a shared library.
shapetcl.so: shapetcl.o $(SHAPELIB_OBJS)
	$(CC) -o shapetcl.so shapetcl.o $(SHAPELIB_OBJS) -shared -L$(TCL_LIBRARY_DIR) -ltclstub8.5

# clean: Remove shared library and compiled object files.
clean:
	rm -f shapetcl.so shapetcl.o $(SHAPELIB_OBJS)

# test: Run all scripts in the test suite.
test: shapetcl.so
	$(TCL) tests/all.tcl

# doc: Convert documentation into various formats.
doc:
	cd doc && $(TCL) FormatDocs.tcl
	cd doc && /usr/bin/html2markdown --body-width=0 <shapetcl.html >shapetcl.md
