package require Tcl 8.5
package require tcltest 2
namespace import ::tcltest::test ::tcltest::makeFile ::tcltest::removeFile

# Use the tests directory as the working directory for all tests.
::tcltest::workingDirectory [file dirname [info script]]

# Stow any temporary test files in the tmp subdirectory.
::tcltest::configure -tmpdir tmp

# Apply any additional configuration arguments.
eval ::tcltest::configure $argv

::tcltest::testConstraint splintAvailable [file exists /usr/local/bin/splint]
::tcltest::testConstraint cppcheckAvailable [file exists /usr/bin/cppcheck]

test static-1.0 {
# Check that Splint reports no issues (-weak mode).
} -constraints {
	splintAvailable
} -body {
	# +unix-lib allows splint to see strcasecmp, unlike default ansi-lib
	# +quiet suppresses "herald" line and error count (simplifies success case)
	exec /usr/local/bin/splint -weak +unix-lib +quiet -I../shapelib ../shapetcl.c
} -result {}

test static-2.0 {
# Check that CPPCheck reports no issues.
} -constraints {
	cppcheckAvailable
} -body {
	# -D SAOffset instructs cppcheck not to worry how SAOffset is defined in Shapelib's shapefil.h.
	exec /usr/bin/cppcheck --enable=all -I ../shapelib --quiet -D SAOffset ../shapetcl.c
} -result {}

::tcltest::cleanupTests
