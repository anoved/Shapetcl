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

test polygons-1.0 {
# assert that xy polygon parts are closed (matching first and last vertices)
} -body {
	set shp [shapefile sample/xy/polygon readonly]
	foreach feature [$shp coordinates read] {
		foreach part $feature {
			set xa [lindex $part 0]
			set ya [lindex $part 1]
			set xb [lindex $part end-1]
			set yb [lindex $part end]
			if {$xa != $xb || $ya != $yb} {
				puts "$xa, $ya - $xb, $yb"
			}
		}
	}
	$shp close
} -result {} -output {}


test polygons-1.1 {
# assert that xym polygonm parts are closed (matching first and last vertices)
} -body {
	set shp [shapefile sample/xym/polygonm readonly]
	foreach feature [$shp coordinates read] {
		foreach part $feature {
			set xa [lindex $part 0]
			set ya [lindex $part 1]
			# [lindex $part 2] is ma
			set xb [lindex $part end-2]
			set yb [lindex $part end-1]
			# [lindex $part end] is mb
			if {$xa != $xb || $ya != $yb} {
				puts "$xa, $ya - $xb, $yb"
			}
		}
	}
	$shp close
} -result {} -output {}

test polygons-1.2 {
# assert that xyzm polygonz parts are closed (matching first and last vertices, including z)
} -body {
	set shp [shapefile sample/xyzm/polygonz readonly]
	foreach feature [$shp coordinates read] {
		foreach part $feature {
			set xa [lindex $part 0]
			set ya [lindex $part 1]
			set za [lindex $part 2]
			# [lindex $part 3] is ma
			set xb [lindex $part end-3]
			set yb [lindex $part end-2]
			set zb [lindex $part end-1]
			# [lindex $part end] is mb
			if {$xa != $xb || $ya != $yb || $za != $zb} {
				puts "$xa, $ya, $za - $xb, $yb, $zb"
			}
		}
	}
	$shp close
} -result {} -output {}

test polygons-1.3 {
# check that xy polygon parts have at least four vertices
} -body {
	set shp [shapefile sample/xy/polygon readonly]
	foreach feature [$shp coordinates read] {
		foreach part $feature {
			set verticeCount [expr {[llength $part] / 2}]
			if {$verticeCount < 4} {
				puts $part
			}
		}
	}
	$shp close
} -result {} -output {}

test polygons-1.4 {
# check that xym polygonm parts have at least four vertices
} -body {
	set shp [shapefile sample/xym/polygonm readonly]
	foreach feature [$shp coordinates read] {
		foreach part $feature {
			set verticeCount [expr {[llength $part] / 3}]
			if {$verticeCount < 4} {
				puts $part
			}
		}
	}
	$shp close
} -result {} -output {}

test polygons-1.5 {
# check that xyzm polygonz parts have at least four vertices
} -body {
	set shp [shapefile sample/xyzm/polygonz readonly]
	foreach feature [$shp coordinates read] {
		foreach part $feature {
			set verticeCount [expr {[llength $part] / 4}]
			if {$verticeCount < 4} {
				puts $part
			}
		}
	}
	$shp close
} -result {} -output {}

test polygons-2.0 {
# check that both forms of the [coord read] actions return the same vertex data
} -body {
	set shp [shapefile sample/xy/polygon readonly]
	set index 0
	foreach featurea [$shp coordinates read] {
		set featureb [$shp coordinates read $index]
		if {$featurea ne $featureb} {
			puts $featurea
		}
		incr index
	}
	$shp close
} -result {} -output {}

test polygons-2.1 {
# try to read a feature with an index to high
} -setup {
	set shp [shapefile sample/xy/polygon readonly]
} -body {
	set count [$shp info count]
	$shp coordinates read $count
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result {invalid feature index *}

test polygons-2.2 {
# try to read a feaure with a non-integer index
} -setup {
	set shp [shapefile sample/xy/polygon readonly]
} -body {
	$shp coordinates read 5.24
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result {expected integer but got *}



::tcltest::cleanupTests
