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

#
# [write] commands
#

test write-1.0 {
# invoke the [write] command with no arguments
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp write
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test write-1.1 {
# invoke the [write] command with too few arguments
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp write foo
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test write-1.2 {
# invoke the [write] command with too many arguments
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp write foo bar soom
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test write-1.3 {
# attempt to use the [write] command with a readonly shapefile
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp write {{0 0}} {}
} -cleanup {
	$shp close
} -returnCodes {
	error
} -result "cannot write to readonly shapefile"

test write-1.4 {
# write a new record
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp write {{0 0}} {0 100.5 "beep beep beep"}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test write-1.5 {
# write a new shape, with explicit null geometry
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp write {} {0 100.5 "beep beep beep"}
	$shp coord read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {}

test write-1.6 {
# write a new shape, with an explicitly null attribute record
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp write {{1 1}} {}
	$shp attr read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {{} {} {}}

test write-1.7 {
# write a new shape, with explicitly null geometry and record (just because)
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp write {} {}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test write-1.8 {
# invoke [write] with an invalid attribute value (pre-validation test)
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp write {{1 1}} {123456789012 0.0 "int too big"}
} -cleanup {
	# info count checks shp and dbf counts; if mismatched, will throw error
	$shp info count
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "integer value too large to represent"

test write-1.9 {
# invoke [write] with invalid coordinate geometry
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp write {{0 0 42}} {0 0.0 "extra coordinate"}
} -cleanup {
	$shp info count
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "2 coordinate values are expected for each vertex"

::tcltest::cleanupTests
