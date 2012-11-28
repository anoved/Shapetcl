package require Tcl 8.5
package require tcltest 2
namespace import ::tcltest::test ::tcltest::makeFile ::tcltest::removeFile

# Use the tests directory as the working directory for all tests.
::tcltest::workingDirectory [file dirname [info script]]

# Stow any temporary test files in the tmp subdirectory.
::tcltest::configure -tmpdir tmp

# Apply any additional configuration arguments.
eval ::tcltest::configure $argv

# Load the extension to test.
lappend auto_path ..
package require Shapetcl

test exponent-1.0 {
} -body {
	
	set out [shapefile tmp/exp point {double Value 16 6}]
	
	# this value fits in width 16 with decimal and six decimal places
	$out attributes write 123456789
	
	# this value does not, and will be stored in scientific notation instead
	$out attributes write 1234567890
	$out close
	
	# although stored differently, both values should be recoverable as doubles
	set in [shapefile tmp/exp readonly]
	puts [$in attributes read]
	$in close
	
} -cleanup {
	file delete tmp/exp.shp
	file delete tmp/exp.shx
	file delete tmp/exp.dbf
} -returnCodes {
	ok
} -output {123456789.0 1234567890.0
} -result {}

::tcltest::cleanupTests
