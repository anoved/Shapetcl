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

test config-1.0 {
# invoke config command with too many arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp config readRawStrings 1 foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result {wrong # args: *}

test config-1.0b {
# invoke config command with too few arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp config
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result {wrong # args: *}

test config-1.1 {
# invoke config command with invalid option name
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp config goGoGadget
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result {bad option *}

test config-1.2 {
# invoke config command with invalid option value
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp config readRawStrings -1
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result {invalid option value *}

# note that allowAlternateNotation only makes a difference when writing
# attribute values, but we allow setting config option regardless of readonly
test config-1.3 {
# check allowAlternateNotation default value and confirm config set
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	# print default - expected to be 0
	puts [$shp config allowAlternateNotation]
	
	# override default and confirm value is now 1
	$shp config allowAlternateNotation 1
	puts [$shp config allowAlternateNotation]
} -cleanup {
	$shp close
} -result {} -output {0
1
}

test config-1.4 {
# check getAllCoordinates & getOnlyXyCoordinates defaults and interaction
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	# expected to be 0
	puts [$shp config getAllCoordinates]
	
	# expected to be 0
	puts [$shp config getOnlyXyCoordinates]
	
	# set getAllCoordinates true and confirm
	$shp config getAllCoordinates 1
	puts [$shp config getAllCoordinates]
	
	# set getOnlyXyCoordinates true and confirm
	$shp config getOnlyXyCoordinates 1
	puts [$shp config getOnlyXyCoordinates]
	
	# true values of getAll and getOnlyXy are mutually exclusive; setting one
	# should disable the other if previously true. getAll should now be false:
	puts [$shp config getAllCoordinates]
	
	# set getAll true again, and confirm that it bumps getOnlyXy back to false:
	$shp config getAllCoordinates 1
	puts [$shp config getOnlyXyCoordinates]
} -cleanup {
	$shp close
} -result {} -output {0
0
1
1
0
0
}

test config-1.5 {
# check readRawStrings default and confirm config set
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	# expected to be 0
	puts [$shp config readRawStrings]
	
	# set to true and confirm
	$shp config readRawStrings 1
	puts [$shp config readRawStrings]
} -cleanup {
	$shp close
} -result {} -output {0
1
}

test config-1.6 {
# check autoClosePolygons default and confirm config set
} -setup {
	set shp [shapefile sample/xy/polygon readonly]
} -body {
	# expected to be 0
	puts [$shp config autoClosePolygons]
	
	# set to true and confirm
	$shp config autoClosePolygons 1
	puts [$shp config autoClosePolygons]
} -cleanup {
	$shp close
} -result {} -output {0
1
}

test config-1.7 {
# check allowTruncation default and confirm config set
} -setup {
	set shp [shapefile sample/xy/polygon readonly]
} -body {
	# expected to be 0
	puts [$shp config allowTruncation]
	
	# set to true and confirm
	$shp config allowTruncation 1
	puts [$shp config allowTruncation]
} -cleanup {
	$shp close
} -result {} -output {0
1
}

test config-2.0 {
# confirm function of allowAlternateNotation config option (exponent-1.3)
} -setup {
	set out [shapefile tmp/config-2-0 point {double Value 16 6}]
} -body {
	# succeeds, since allowAlternateNotation is on by default
	$out attributes write 1234567890.123456
	
	# throws error if allowAlternateNotation is set to false
	$out config allowAlternateNotation 0
	$out attributes write 1234567890.123456
} -cleanup {
	$out close
	file delete {*}[glob -nocomplain tmp/config-2-0.*]
} -returnCodes {
	error
} -match glob -result {failed to write double attribute "*"}

test config-2.1 {
# confirm function of getAllCoordinates config option
} -setup {
	set shp [shapefile sample/xym/pointm readonly]
} -body {
	# by default, we expect 3 coordinates per vertex here
	puts [llength [lindex [$shp coord read 0] 0]]
	
	# with getAllCoordinates, we expect 4 coordinates per vertex
	$shp config getAllCoordinates 1
	puts [llength [lindex [$shp coord read 0] 0]]
} -cleanup {
	$shp close
} -result {} -output {3
4
}

test config-2.2 {
# confirm function of getOnlyXyCoordinates config option
} -setup {
	set shp [shapefile sample/xym/pointm readonly]
} -body {
	# by default, we expect 3 coordinates per vertex here
	puts [llength [lindex [$shp coord read 0] 0]]

	# with getOnlyXyCoordinates, we expect 2 coordinates per vertex
	$shp config getOnlyXyCoordinates 1
	puts [llength [lindex [$shp coord read 0] 0]]
} -cleanup {
	$shp close
} -result {} -output {3
2
}

test config-2.3 {
# confirm function of readRawStrings config option
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	# 6691831.0
	puts [$shp attr read 0 31]
	
	$shp config readRawStrings 1
	# 6691831.00000000000
	puts [$shp attr read 0 31]
} -cleanup {
	$shp close
} -result {} -output {6691831.0
6691831.00000000000
}

test config-2.4 {
# confirm function of autoClosePolygons config option (see coord-7.4 & 7.5)
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0}]
} -body {
	$shp config autoClosePolygons 1
	$shp coord write {{0 10  10 10  10 0  0 0}}
	$shp coord read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {{0.0 10.0 10.0 10.0 10.0 0.0 0.0 0.0 0.0 10.0}}

test config-2.5 {
# confirm function of allowTruncation config option with too-large integer
} -setup {
	set out [shapefile tmp/config-2-5 point {integer Value 5 0}]
} -body {
	$out configure allowTruncation 1
	$out attributes write 123456
	$out attributes read 0 0
} -cleanup {
	$out close
	file delete {*}[glob -nocomplain tmp/config-2-5.*]
} -result {12345}

test config-2.6 {
# confirm function of allowTruncation config option with too-large double
} -setup {
	set out [shapefile tmp/config-2-6 point {double Value 16 6}]
} -body {
	# if allowAlternateNotation is left on, it would handle this (see config-2.0)
	$out config allowAlternateNotation 0
	
	# the trailing digit 6 will be truncated
	$out config allowTruncation 1
	$out attributes write 1234567890.123456
	$out attributes read 0 0
} -cleanup {
	$out close
	file delete {*}[glob -nocomplain tmp/config-2-6.*]
} -result {1234567890.12345}

test config-2.7 {
# confirm function of allowTruncation config option with too-large string
} -setup {
	set out [shapefile tmp/config-2-7 point {string Value 5 0}]
} -body {
	$out config allowTruncation 1
	$out attributes write "abcdef"
	$out attributes read 0 0
} -cleanup {
	$out close
	file delete {*}[glob -nocomplain tmp/config-2-7.*]
} -result {abcde}

::tcltest::cleanupTests
