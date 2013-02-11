package require Tcl 8.5
package require tcltest 2
namespace import ::tcltest::test ::tcltest::makeFile ::tcltest::removeFile

# Use the tests directory as the working directory for all tests.
::tcltest::workingDirectory [file dirname [info script]]

# Stow any temporary test files in the tmp subdirectory.
::tcltest::configure -tmpdir tmp

# Apply any additional configuration arguments.
eval ::tcltest::configure $argv

package require platform

::tcltest::testConstraint otoolAvailable \
		[expr {[string match macosx-* [::platform::generic]]
		&& ![catch {exec which otool}]}]

::tcltest::testConstraint lddAvailable \
		[expr {![catch {exec which ldd}]}]

test stubs-1.0 {
# confirm that the library appears stubs-compatible on Mac OS X
} -constraints {
	otoolAvailable
} -body {
	set report [exec otool -L ../shapetcl.so]
	string match *Tcl.framework* $report
} -result {0}

test stubs-1.1 {
# confirm that the library appears stub-compatible on *nix
} -constraints {
	lddAvailable
} -body {
	set report [exec ldd ../shapetcl.so]
	string match *libtcl* $report
} -result {0}

::tcltest::cleanupTests
