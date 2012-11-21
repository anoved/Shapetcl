package require Tcl 8.5
package require tcltest 2
namespace import ::tcltest::test

# Use the tests directory as the working directory for all tests.
::tcltest::workingDirectory [file dirname [info script]]

# Stow any temporary test files in the tmp subdirectory.
::tcltest::configure -tmpdir tmp

# Apply any additional configuration arguments.
eval ::tcltest::configure $argv

# Load the extension to test.
lappend auto_path ..
package require Shapetcl

# Tests:


test "shapefile-cmd-1.0" {
Test what happens when the shapefile command is invoked with no arguments.
} -body {
	shapefile
} -returnCodes {
	error
} -result {wrong # args: should be "shapefile path ?mode?|?type fields?"}


::tcltest::cleanupTests
