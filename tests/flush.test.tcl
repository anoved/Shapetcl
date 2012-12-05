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

test flush-1.0 {
# Test that flush command writes changes to disk and shapefile remains open.
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
} -body {
	
	# open an existing shapefile
	set shp [shapefile tmp/point readwrite]
	
	# report how many features are in file initially
	puts [$shp info count]
	
	# make a change (add a point, with null attributes implied)
	$shp coordinates write {{0 0}}
	
	# flush changes to disk
	$shp flush

	# independently check how many features are now in file
	# (alternatively, use an external utility like shpdump)
	set check [shapefile tmp/point readonly]
	puts [$check info count]
	$check close
	
	# confirm that we see the same count w/original handle,
	# indicating the files were reopened ok.
	puts [$shp info count]
	
	$shp close
	
} -cleanup {
	file delete {*}[glob -nocomplain tmp/point.*]
} -returnCodes {
	ok
} -result {} -output {243
244
244
}

test flush-1.1 {
# invoke [flush] command with too many arguments
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	$shp flush foo
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -returnCodes {
	error
} -match glob -result "wrong # args: *"


test flush-1.2 {
# confirm that [flush] does nothing with readonly shapefiles
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp flush
} -cleanup {
	$shp close
} -result {}

# note that deleting the files while open is likely to cause problems for many
# commands, not just flush. Flush is the only command that explicitly relies on
# reopening the given path, though.
test flush-1.3 {
# attempt to cause [flush] to fail by deleting open files
} -setup {
	
	set shp [shapefile tmp/foo point {integer Value 10 0}]
	$shp write {{1 1}} {100}
	$shp flush
	
	# delete the files while still open
	file delete tmp/foo.shp
	file delete tmp/foo.shx
	file delete tmp/foo.dbf

} -body {

	# attempt to close and reopen the now-missing files
	$shp flush
	
} -returnCodes {
	error
} -match glob -result "cannot reopen flushed shapefile *"

::tcltest::cleanupTests
