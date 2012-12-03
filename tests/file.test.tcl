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

test file-1.0 {
# invoke file cmd with too few arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp file
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result {wrong # args: *}

test file-1.1 {
# invoke file cmd with too many arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp file foo bar
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result {wrong # args: *}

test file-1.2 {
# invoke file cmd with invalid option
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp file foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result {bad option *}

test file-1.3 {
# confirm file mode reports default readwrite mode
} -setup {
	set shp [shapefile sample/xy/point]
} -body {
	$shp file mode
} -cleanup {
	$shp close
} -result {readwrite}

test file-1.4 {
# confirm file mode reports explicit readwrite mode
} -setup {
	set shp [shapefile sample/xy/point readwrite]
} -body {
	$shp file mode
} -cleanup {
	$shp close
} -result {readwrite}

test file-1.5 {
# confirm file mode reports explicit readonly mode
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp file mode
} -cleanup {
	$shp close
} -result {readonly}

test file-1.6 {
# confirm file path reports specified base path
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp file path
} -cleanup {
	$shp close
} -result {sample/xy/point}

test file-1.7 {
# confirm file path reports base path given .shp suffix
} -setup {
	set shp [shapefile sample/xy/point.shp readonly]
} -body {
	$shp file path
} -cleanup {
	$shp close
} -result {sample/xy/point}

test file-1.8 {
# confirm file path reports base path given .shx suffix
} -setup {
	set shp [shapefile sample/xy/point.shx readonly]
} -body {
	$shp file path
} -cleanup {
	$shp close
} -result {sample/xy/point}

test file-1.9 {
# confirm file path reports base path given .dbf suffix
} -setup {
	set shp [shapefile sample/xy/point.dbf readonly]
} -body {
	$shp file path
} -cleanup {
	$shp close
} -result {sample/xy/point}

test file-1.10 {
# confirm file path reports base path given... any suffix (a shapelib "feature")
} -setup {
	set shp [shapefile sample/xy/point.bsg readonly]
} -body {
	$shp file path
} -cleanup {
	$shp close
} -result {sample/xy/point}

# lots of other path variations to consider, including filesystem tricks.

::tcltest::cleanupTests
