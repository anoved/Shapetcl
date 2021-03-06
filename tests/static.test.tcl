package require Tcl 8.5
package require tcltest 2
namespace import ::tcltest::test ::tcltest::makeFile ::tcltest::removeFile

# Use the tests directory as the working directory for all tests.
::tcltest::workingDirectory [file dirname [info script]]

# Stow any temporary test files in the tmp subdirectory.
::tcltest::configure -tmpdir tmp

# Apply any additional configuration arguments.
eval ::tcltest::configure $argv

# Because these tests are more time consuming than others, they are skipped
# by default. To run them, run `tclsh tests/static.test.tcl -constraints static`

::tcltest::testConstraint splintAvailable [expr {![catch {exec which splint}]}]
::tcltest::testConstraint cppcheckAvailable [expr {![catch {exec which cppcheck}]}]
::tcltest::testConstraint clangAvailable [expr {![catch {exec which clang}]}]

test static-1.0 {
# Check that Splint reports no issues (-weak mode).
} -constraints {
	static
	splintAvailable
} -body {
	# +unix-lib allows splint to see strcasecmp, unlike default ansi-lib
	# +quiet suppresses "herald" line and error count (simplifies success case)
	exec splint -weak +unix-lib +quiet -I../shapelib -I/usr/include/tcl8.5 ../shapetcl.c
} -result {}

test static-2.0 {
# Check that CPPCheck reports no issues.
} -constraints {
	static
	cppcheckAvailable
} -body {
	# -D SAOffset instructs cppcheck not to worry how SAOffset is defined in Shapelib's shapefil.h.
	exec cppcheck --enable=all -I ../shapelib --quiet -D SAOffset ../shapetcl.c
} -result {}

test static-3.0 {
# Check that Clang's analyzer reports no issues.
} -constraints {
	static
	clangAvailable
} -body {
	exec clang -I../shapelib --analyze ../shapetcl.c
} -cleanup {
	# (empty diagnostics file generated by clang)
	file delete shapetcl.plist
} -result {}

::tcltest::cleanupTests
