package require Tcl 8.5
package require tcltest 2
namespace import ::tcltest::test ::tcltest::makeFile ::tcltest::removeFile

# Use the tests directory as the working directory for all tests.
::tcltest::workingDirectory [file dirname [info script]]

# Stow any temporary test files in the tmp subdirectory.
::tcltest::configure -tmpdir tmp

# Apply any additional configuration arguments.
eval ::tcltest::configure $argv

test stubs-1.0 {
# confirm that the Shapetcl library appears to be stubs-compatible
} -body {
	# (try dumpbin /dependents on windows?)
	if {[file exists ../Shapetcl.dylib]} {
		# mac os x
		set report [exec /usr/bin/otool -L ../Shapetcl.dylib]
	} elseif {[file exists ../Shapetcl.so]} {
		# linux/unix
		set report [exec /usr/bin/ldd ../Shapetcl.so]
	} else {
		error "No Shapetcl library to test"
	}
	# A stubs-compatible extension should not reference Tcl libraries directly.
	if {	[string match *libtcl* $report] ||
			[string match *Tcl.framework* $report]} {
		error "Shapetcl not stubs compatible"
	}
} -result {}

::tcltest::cleanupTests
