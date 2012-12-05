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
# [coordinates] command
#

test coord-1.0 {
# invoke [coord] command with too few arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp coord
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test coord-1.1 {
# invoke [coord] command with invalid action
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp coord foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "bad action *"

#
# [coord read] action
#

test coord-2.0 {
# invoke [coord read] with too many arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp coord read 0 foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test coord-2.1 {
# invoke [coord read index] with non-integer index
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp coord read foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "expected integer but got *"

test coord-2.1b {
# invoke [coord read index] with invalid (too high) index
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp coord read 300
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "invalid feature index *"

test coord-2.2 {
# confirm [coord read index] with point feature
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp coord read 0
} -cleanup {
	$shp close
} -result {{12.453386544971766 41.903282179960115}}

test coord-2.3 {
# confirm [coord read index] with pointm feature
} -setup {
	set shp [shapefile sample/xym/pointm readonly]
} -body {
	$shp coord read 0
} -cleanup {
	$shp close
} -result {{12.453386544971766 41.903282179960115 0.0}}

test coord-2.4 {
# confirm [coord read index] with pointz feature
} -setup {
	set shp [shapefile sample/xyzm/pointz readonly]
} -body {
	$shp coord read 0
} -cleanup {
	$shp close
} -result {{12.453386544971766 41.903282179960115 39.77220986959162 0.0}}

test coord-2.5 {
# try to read a null feature with [coord read index]
} -setup {
	set shp [shapefile tmp/foo point {integer Id 10 0}]
	# writes null feature geometry, normally to be replaced w/coord write
	$shp write {} {0}
} -body {
	$shp coord read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {}

# polygon-specific verification of [coord read] is found in polygons.test.tcl

test coord-2.6 {
# confirm [coord read index] returns all multipoint vertices in one part
} -setup {
	set shp [shapefile sample/xy/multipoint readonly]
} -body {
	llength [$shp coord read 0]
} -cleanup {
	$shp close
} -result {1}

test coord-2.7 {
# confirm [coord read] returns the expected number of coordinate lists
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	llength [$shp coord read]
} -cleanup {
	$shp close
} -result {243}

#
# [coord write] action
#

test coord-3.0 {
# invoke [coord write] with no arguments
} -setup {
	set shp [shapefile tmp/foo point {integer Id 10 0}]
} -body {
	$shp coord write
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test coord-3.1 {
# invoke [coord write] with too many arguments
} -setup {
	set shp [shapefile tmp/foo point {integer Id 10 0}]
} -body {
	$shp coord write 0 {{0 0}} foo
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test coord-3.2 {
# invoke [coord write index {geometry}] with non-integer index
} -setup {
	set shp [shapefile tmp/foo point {integer Id 10 0}]
} -body {
	$shp coord write foo {{0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "expected integer but got *"

test coord-3.3 {
# invoke [coord write index {geometry}] with invalid index (too high)
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point readwrite]
} -body {
	$shp coord write 300 {{0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/point.*]
} -returnCodes {
	error
} -match glob -result "invalid feature index *"

test coord-3.4 {
# invoke [coord write index {geometry}] with invalid index (-1)
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point readwrite]
} -body {
	# index -1 is used internally to tell cmd_attribute_write to append a new
	# feature when calling [coord write {geometry}], but is not allowed here.
	$shp coord write -1 {{0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/point.*]
} -returnCodes {
	error
} -match glob -result "invalid feature index *"

test coord-3.5 {
# confirm that [coord write index {geometry}] won't work with readonly files
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp coord write 0 {{0 0}}
} -cleanup {
	$shp close
} -returnCodes {
	error
} -result "cannot write coordinates to readonly shapefile"

test coord-3.6 {
# overwrite a feature with a null feature and confirm
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point readwrite]
} -body {

	# output the original coordinates
	puts [$shp coord read 0]
	
	# overwrite with a null feature
	$shp coord write 0 {}
	
	# output again, now as a null feature
	puts [$shp coord read 0]

} -cleanup {
	$shp close
	file delete {*}[glob tmp/point.*]
} -result {} -output {{12.453386544971766 41.903282179960115}

}

test coord-3.6 {
# verify [coord read] returns null feature list suitable for [coord write]
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point readwrite]
} -body {

	# make a null feature explicitly
	$shp coord write 0 {}
	
	# make a null feature implicitly, by passing coordinate list of a null
	$shp coord write 1 [$shp coord read 0]
	
	# return the implicit null feature coordinate list
	$shp coord read 1

} -cleanup {
	$shp close
	file delete {*}[glob tmp/point.*]
} -result {}

#
# numerous additional tests needed to check coordinate list formatting, etc.
# perhaps also some stress-testing with large numbers of vertices, etc.
# 
#

::tcltest::cleanupTests
