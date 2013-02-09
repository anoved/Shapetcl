#!/usr/bin/tclsh

package require doctools
::doctools::new doc

# read the raw documentation
set f [open shapetcl.dtp]
set dt [read $f]
close $f

# generate html format
doc configure -format html
set f [open shapetcl.html w]
puts $f [doc format $dt]
close $f

# generate plain text format
doc configure -format text
set f [open shapetcl.txt w]
puts $f [doc format $dt]
close $f
