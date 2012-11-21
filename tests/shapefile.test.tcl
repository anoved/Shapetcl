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

# Tests:


test shapefile-1.0 {
# Test what happens when the shapefile command is invoked with no arguments.
} -body {
	shapefile
} -returnCodes {
	error
} -result {wrong # args: should be "shapefile path ?mode?|?type fields?"}

test shapefile-1.0.1 {
# Test what happens when the shapefile command is invoked with too many arguments.
} -body {
	shapefile foo type fields extra
} -returnCodes {
	error
} -result {wrong # args: should be "shapefile path ?mode?|?type fields?"}

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
} -constraints {
	emptyTest
}

test shapefile-1.5 {
# Attempt to open a shapefile with a valid attribute table but no shp/shx files
} -setup {
	file copy sample/ne_110m_land/ne_110m_land.dbf tmp/foo.dbf
} -body {
	shapefile tmp/foo
} -cleanup {
	file delete tmp/foo.dbf
} -returnCodes {
	error
} -match glob -result {failed to open shapefile for "*"}
# Out of the box, Shapelib fprintfs certain error messages directly to stderr.
# This is inconvenient given its role as library - the caller might prefer to
# handle or report errors otherwise. These C-level stderr messages cannot be
# caught by tcltest's -errorOutput option, so they cause the test file to fail.
# So, I have commented out the fprintf() statement in SADError() in safileio.c.

test shapefile-1.6 {
# Attempt to open a shapefile with a valid attribute table and shp/shx files that are not valid
} -constraints {
	emptyTest
}

test shapefile-1.7 {
# Attempt to open a shapefile that has valid files but mismatched number of records
} -constraints {
	emptyTest
}

test shapefile-1.8 {
# Attempt to open a valid shapefile of an unsupported type
} -constraints {
	emptyTest
}

test shapefile-1.9 {
# Attempt to open a valid shapefile
} -body {
	set shp [shapefile sample/ne_110m_land/ne_110m_land]
} -cleanup {
	$shp close
} -match glob -result {shapefile.*}

::tcltest::cleanupTests
