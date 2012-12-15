#!/usr/bin/tclsh
package require doctools
::doctools::new doc -format html
puts [doc format [read stdin]]
