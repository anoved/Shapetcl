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
# Check that double values stored in scientific notation can be read ok.
} -setup {
	set out [shapefile tmp/exp0 point {double Value 16 6}]
	# this value fits in width 16 with decimal and six decimal places:
	$out attributes write 123456789
	# this value does not, and will be stored in scientific notation instead:
	$out attributes write 1234567890
	$out close
} -body {
	# although stored differently, both values should be recoverable as doubles
	set in [shapefile tmp/exp0 readonly]
	$in attributes read
} -cleanup {
	$in close
	file delete {*}[glob -nocomplain tmp/exp0.*]
} -returnCodes {
	ok
} -result {123456789.0 1234567890.0}

test exponent-1.1 {
# Illustrate loss of significant digits when storing large high-precision values
# (1234567890.123456's 10 digits left-of-decimal are too wide for 16.6 format,
# so it is stored as " 1.234567890e+09". The right-of-decimal digits are lost.)
} -setup {
	set out [shapefile tmp/exp1 point {double Value 16 6}]
	$out attributes write 1234567890.123456
	$out close
} -body {
	set in [shapefile tmp/exp1 readonly]
	$in attributes read 0
} -cleanup {
	$in close
	file delete {*}[glob -nocomplain tmp/exp1.*]
} -returnCodes {
	ok
} -result {1234567890.0}

# If we're willing to drop digits to fit large values for scientific notation,
# consider that we should be able to fit this value (1000) in 6 digits, too.
test exponent-1.2 {
# Check that large values cannot be written either way to small fields (XXX.XX).
} -setup {
	set out [shapefile tmp/exp2 point {double WeeValue 6 2}]
} -body {
	$out attributes write 1000
} -cleanup {
	$out close
	file delete {*}[glob -nocomplain tmp/exp2.*]
} -returnCodes {
	error
} -match glob -result {field too narrow (*) for fixed or scientific notation representation of value "*"}

::tcltest::cleanupTests
