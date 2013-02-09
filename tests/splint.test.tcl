package require Tcl 8.5
package require tcltest 2
namespace import ::tcltest::test ::tcltest::makeFile ::tcltest::removeFile

# Use the tests directory as the working directory for all tests.
::tcltest::workingDirectory [file dirname [info script]]

# Stow any temporary test files in the tmp subdirectory.
::tcltest::configure -tmpdir tmp

# Apply any additional configuration arguments.
eval ::tcltest::configure $argv

# This file contains tests that check the Shapetcl source code (shapetcl.c)
# for problems reported by Secure Programming Lint (http://www.splint.org/).

::tcltest::testConstraint splintAvailable [file exists /usr/local/bin/splint]

test splint-1.0 {
# Check that Splint reports no issues (-weak mode).
} -constraints {
	splintAvailable
} -body {
	# +unix-lib allows splint to see strcasecmp, unlike default ansi-lib
	# +quiet suppresses "herald" line and error count (simplifies success case)
	exec /usr/local/bin/splint -weak +unix-lib +quiet -I../shapelib ../shapetcl.c
} -result {}

::tcltest::cleanupTests
