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
package require shapetcl

# careful to unset variables and delete namespaces here to ensure that
# subsequent tests can accurately determine whether variables are visible
# (otherwise var ::shp would make ::foo::shp visible even if not exported)

test ns-1.0 {
# attempt to invoke shapefile cmd w/out import or fully qualified namespace
} -body {
	shapefile sample/xy/point readonly
} -returnCodes {
	error
} -match glob -result "invalid command name *"

test ns-1.1 {
# invoke shapefile cmd w/fully qualified name
} -body {
	set shp [::shapetcl::shapefile sample/xy/point readonly]
} -cleanup {
	$shp close
	unset shp
} -result "::shapefile0"

test ns-1.2 {
# open shapefile in a sub ns; confirm cmd is not accessible globally
} -setup {
	namespace eval foo {
		set bar [::shapetcl::shapefile sample/xy/point readonly]
	}
} -body {
	$bar info count
} -cleanup {
	namespace eval foo {
		$bar close
	}
	namespace delete foo
} -returnCodes {
	error
} -match glob -result "can't read *: no such variable"

test ns-1.3 {
# access shapefile in namespace eval
} -setup {
	namespace eval foo {
		set shp [::shapetcl::shapefile sample/xy/point readonly]
	}
} -body {
	namespace eval foo {
		$shp info count
	}
} -cleanup {
	namespace eval foo {
		$shp close
	}
	namespace delete foo
} -result {243}
	
::tcltest::cleanupTests
