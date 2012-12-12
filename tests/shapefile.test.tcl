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
namespace import shapetcl::shapefile

#
# Testing for "Error" Conditions
#

test shapefile-1.0 {
# Test what happens when the shapefile command is invoked with no arguments.
} -body {
	shapefile
} -returnCodes {
	error
} -match glob -result {wrong # args: should be "*"}

test shapefile-1.0.1 {
# Test what happens when the shapefile command is invoked with too many arguments.
} -body {
	shapefile foo type fields extra
} -returnCodes {
	error
} -match glob -result {wrong # args: should be "*"}

test shapefile-1.1 {
# Attempt to open a file that does not exist.
} -body {
	shapefile foo
} -returnCodes {
	error
} -match glob -result {failed to open attribute table for *}

test shapefile-1.2 {
# Attempt to open a single file that exists but is not a shapefile.
} -setup {
	set f [makeFile {} foo]
} -body {
	shapefile $f
} -cleanup {
	removeFile foo
} -returnCodes {
	error
} -match glob -result {failed to open attribute table for *}

test shapefile-1.2 {
# Attempt to open a set of files named like a shapefile but which are not a shapefile.
} -setup {
	set f [makeFile {} foo.shp]
	makeFile {} foo.shx
	makeFile {} foo.dbf
} -body {
	shapefile $f
} -cleanup {
	removeFile foo.shp
	removeFile foo.shx
	removeFile foo.dbf
} -returnCodes {
	error
} -match glob -result {failed to open attribute table for *}

test shapefile-1.3 {
# Attempt to open a file using an invalid access mode
} -body {
	shapefile foo invalidmode
} -returnCodes {
	error
} -match glob -result {invalid mode "*": should be readonly or readwrite}

test shapefile-1.4 {
# Attempt to open a shapefile with an attribute table that exists but contains no fields
} -body {
	shapefile sample/misc/empty
} -returnCodes {
	error
} -match glob -result {attribute table for "*" contains no fields}

test shapefile-1.5 {
# Attempt to open a shapefile with a valid attribute table but no shp/shx files
} -setup {
	file copy sample/xy/point.dbf tmp/foo.dbf
} -body {
	shapefile tmp/foo
} -cleanup {
	file delete tmp/foo.dbf
} -returnCodes {
	error
} -match glob -result {failed to open shapefile for "*"}

test shapefile-1.6 {
# Attempt to open a shapefile with a valid attribute table and shp/shx files that are not valid
} -setup {
	file copy sample/xy/point.dbf tmp/foo.dbf
	makeFile {} foo.shp
	makeFile {} foo.shx
} -body {
	shapefile tmp/foo
} -cleanup {
	file delete tmp/foo.dbf
	removeFile foo.shp
	removeFile foo.shx
} -returnCodes {
	error
} -match glob -result {failed to open shapefile for "*"}

test shapefile-1.7 {
# Attempt to open a shapefile that has valid files but mismatched number of records
} -body {
	shapefile sample/misc/mismatch
} -returnCodes {
	error
} -match glob -result {shapefile feature count (*) does not match attribute record count (*)}
	
test shapefile-1.8 {
# Attempt to open a valid shapefile of an as-yet unsupported type (multipatch)
} -body {
	shapefile sample/misc/multipatch readonly
} -returnCodes {
	error
} -match glob -result "unsupported shape type*"

test shapefile-1.9 {
# Attempt to invoke an invalid subcommand
} -setup {
	set shp [shapefile sample/xy/point]
} -body {
	$shp funk
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result {bad subcommand "*": must be *}

test shapefile-1.10 {
# Attempt to open a valid (polygon) shapefile
} -body {
	set shp [shapefile sample/xy/polygon]
} -cleanup {
	$shp close
} -returnCodes {
	ok
} -match glob -result {shapefile*}

test shapefile-1.11 {
# Verify that by default shapefiles are opened in readwrite mode
} -setup {
	set shp [shapefile sample/xy/point]
} -body {
	$shp file mode
} -cleanup {
	$shp close
} -returnCodes {
	ok
} -match exact -result {readwrite}

test shapefile-1.12 {
# attempt to create a shapefile of a valid but unsupported type
} -body {
	shapefile tmp/foo multipatch {integer id 10 0}
} -returnCodes {
	error
} -match glob -result "unsupported shape type*"
	

test shapefile-1.12 {
# attempt to create a shapefile of an invalid type
} -body {
	shapefile tmp/foo bar {integer id 10 0}
} -returnCodes {
	error
} -result "unrecognized shape type"

::tcltest::cleanupTests
