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

test stubs-1.0 {
# confirm that the Shapetcl library appears to be stubs-compatible
} -body {
	if {[string match macosx-* [::platform::generic]]} {
		# use otool to list shared libraries on Mac OS X
		set report [exec /usr/bin/otool -L ../shapetcl.so]
	} else {
		# use ldd to list shared libraries on *nix
		set report [exec /usr/bin/ldd ../shapetcl.so]
	}
	# A stubs-compatible extension should not reference Tcl libraries directly.
	if {[string match *libtcl* $report] ||
			[string match *Tcl.framework* $report]} {
		error "Shapetcl not stubs compatible"
	}
} -result {}

::tcltest::cleanupTests
