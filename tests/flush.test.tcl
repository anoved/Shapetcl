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
	file copy sample/xy/point.dbf tmp/foo.dbf
	file copy sample/xy/point.shx tmp/foo.shx
	file copy sample/xy/point.shp tmp/foo.shp
} -body {
	
	# open an existing shapefile
	set shp [shapefile tmp/foo readwrite]
	
	# report how many features are in file initially
	puts [$shp info count]
	
	# make a change (add a point, with null attributes implied)
	$shp coordinates write {{0 0}}
	
	# flush changes to disk
	$shp flush

	# independently check how many features are now in file
	# (alternatively, use an external utility like shpdump)
	set check [shapefile tmp/foo readonly]
	puts [$check info count]
	$check close
	
	# confirm that we see the same count w/original handle,
	# indicating the files were reopened ok.
	puts [$shp info count]
	
	$shp close
	
} -cleanup {
	file delete tmp/foo.dbf
	file delete tmp/foo.shx
	file delete tmp/foo.shp
} -returnCodes {
	ok
} -result {} -output {243
244
244
}

::tcltest::cleanupTests
