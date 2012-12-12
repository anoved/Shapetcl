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
# confirm [coord read] all returns the expected number of coordinate lists
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	llength [$shp coord read]
} -cleanup {
	$shp close
} -result {243}

test coord-2.8 {
# confirm [coord read] all returns empty list as expected
} -setup {
	# create an empty new shapefile
	set shp [shapefile tmp/foo point {integer Id 10 0}]
} -body {
	$shp coord read
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {}

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
# [coord write] point geometry
#

test coord-4.0 {
# attempt to write point with empty part
} -setup {
	set shp [shapefile tmp/foo point {integer Id 10 0}]
} -body {
	$shp coord write {{}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid vertex count *: point features *"

test coord-4.1 {
# attempt to write xy point with too few vertex coordinate
} -setup {
	set shp [shapefile tmp/foo point {integer Id 10 0}]
} -body {
	$shp coord write {{1}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -result "2 coordinate values are expected for each vertex"

test coord-4.2 {
# attempt to write xy point with too many vertex coordinates
} -setup {
	set shp [shapefile tmp/foo point {integer Id 10 0}]
} -body {
	$shp coord write {{1 1 1}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -result "2 coordinate values are expected for each vertex"

test coord-4.3 {
# attempt to write xy point with multiple parts
} -setup {
	set shp [shapefile tmp/foo point {integer Id 10 0}]
} -body {
	$shp coord write {{0 0} {1 1}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid part count *: point and multipoint *"

test coord-4.4 {
# write xy point successful
} -setup {
	set shp [shapefile tmp/foo point {integer Id 10 0}]
} -body {
	$shp coord write {{0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test coord-4.5 {
# attempt to write xym point too few coordinates
} -setup {
	set shp [shapefile tmp/foo pointm {integer Id 10 0}]
} -body {
	$shp coord write {{0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -result "3 coordinate values are expected for each vertex"

test coord-4.6 {
# attempt to write xym point with too many coordinates
} -setup {
	set shp [shapefile tmp/foo pointm {integer Id 10 0}]
} -body {
	$shp coord write {{0 0 0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -result "3 coordinate values are expected for each vertex"

test coord-4.7 {
# attempt to write xym point with too many parts
} -setup {
	set shp [shapefile tmp/foo pointm {integer Id 10 0}]
} -body {
	$shp coord write {{0 0 0} {1 1 1}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid part count *: point and multipoint *"

test coord-4.8 {
# write xym point
} -setup {
	set shp [shapefile tmp/foo pointm {integer Id 10 0}]
} -body {
	$shp coord write {{1 1 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test coord-4.9 {
# attemt to write xyzm point with too few coordinates
} -setup {
	set shp [shapefile tmp/foo pointz {integer Id 10 0}]
} -body {
	$shp coord write {{0 0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -result "4 coordinate values are expected for each vertex"

test coord-4.10 {
# attempt to write xyzm point with too many coordinates
} -setup {
	set shp [shapefile tmp/foo pointz {integer Id 10 0}]
} -body {
	$shp coord write {{0 0 0 0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -result "4 coordinate values are expected for each vertex"

test coord-4.11 {
# attempt to write xyzm point with too many parts
} -setup {
	set shp [shapefile tmp/foo pointz {integer Id 10 0}]
} -body {
	$shp coord write {{0 0 0 0} {1 1 0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid part count *: point and multipoint *"

test coord-4.12 {
# write xyzm point
} -setup {
	set shp [shapefile tmp/foo pointz {integer Id 10 0}]
} -body {
	$shp coord write {{1 1 1 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test coord-4.13 {
# attempt to write xy point with invalid (non-numeric) coordinate value
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0}]
} -body {
	$shp coord write {{10 foo}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "expected floating-point number but got *"

#
# [coord write] arc geometry
#

test coord-5.0 {
# attempt to write arc with empty part (too few vertices)
} -setup {
	set shp [shapefile tmp/foo arc {integer Id 10 0}]
} -body {
	$shp coord write {{}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid vertex count *: arc features *"

test coord-5.1 {
# attempt to write arc with too few vertices (minimum of 2)
} -setup {
	set shp [shapefile tmp/foo arc {integer Id 10 0}]
} -body {
	$shp coord write {{10 10}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid vertex count *: arc features *"

test coord-5.2 {
# attempt to write xy arc with malformed coordinate list (incomplete arc)
} -setup {
	set shp [shapefile tmp/foo arc {integer Id 10 0}]
} -body {
	$shp coord write {{10 10 12}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -result "2 coordinate values are expected for each vertex"

test coord-5.3 {
# attempt to write xy arc with malformed coordinate list
} -setup {
	set shp [shapefile tmp/foo arc {integer id 10 0}]
} -body {
	$shp coord write {{10 10  12 12  9}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -result "2 coordinate values are expected for each vertex"

test coord-5.4 {
# write xy arc
} -setup {
	set shp [shapefile tmp/foo arc {integer id 10 0}]
} -body {
	$shp coord write {{0 0  10 10}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test coord-5.5 {
# attempt to write xy arc with non-numeric coordinate
} -setup {
	set shp [shapefile tmp/foo arc {integer id 10 0}]
} -body {
	$shp coord write {{0 0 foo bar}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "expected floating-point number but got *"

test coord-5.6 {
# write multi-part xy arc
} -setup {
	set shp [shapefile tmp/foo arc {integer id 10 0}]
} -body {
	$shp coord write {{0 0  10 0} {0 5  10 5}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test coord-5.7 {
# zero-length arcs are disallowed by shapefile spec, although arcs may contain
# identical vertices as long as there is at least one different vertex.
# (this constraint is not currently enforced by Shapetcl; tally arc length)
} -constraints {
	emptyTest
}

test coord-5.8 {
# write an xy arc with many vertices
} -setup {
	set shp [shapefile tmp/foo arc {integer id 10 0}]
} -body {
	for {set i 0} {$i < 180} {incr i} {
		lappend vertices $i [expr {$i / 2}]
	}
	$shp coord write [list $vertices]
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test coord-5.9 {
# write an xy arc with many parts
} -setup {
	set shp [shapefile tmp/foo arc {integer id 10 0}]
} -body {
	for {set i 0} {$i < 180} {incr i 2} {
		lappend coords [list $i [expr {$i / 2}] [expr {$i + 1}] [expr {$i / 2}]]
	}
	$shp coord write $coords
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test coord-5.10 {
# attempt to write an xym arc with too few coordinates
} -setup {
	set shp [shapefile tmp/foo arcm {integer id 10 0}]
} -body {
	$shp coord write {{0 0 10 10}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -result "3 coordinate values are expected for each vertex"

# note there are some ambiguous cases - such as six coordinates given for an
# xym arc. do they represent two xym points (valid input), or three xy points
# (invalid input)? Since the feature type is xym, it makes sense to presume xym,
# but it does mean we can't necessarily catch all cases where the application
# provides the wrong number of verticess

test coord-5.11 {
# write an xym arc
} -setup {
	set shp [shapefile tmp/foo arcm {integer id 10 0}]
} -body {
	$shp coord write {{0 0 0  1 1 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test coord-5.12 {
# write an xyzm arc
} -setup {
	set shp [shapefile tmp/foo arcz {integer id 10 0}]
} -body {
	$shp coord write {{0 0 0 0  1 1 1 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

#
# [coord write] multipoint geometry
#

test coord-6.0 {
# attempt to write a multipoint feature with an empty part
} -setup {
	set shp [shapefile tmp/foo multipoint {integer id 10 0}]
} -body {
	$shp coord write {{}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid vertex count *: multipoint *"

test coord-6.1 {
# attempt to write multipoint with multiple parts
} -setup {
	set shp [shapefile tmp/foo multipoint {integer id 10 0}]
} -body {
	$shp coord write {{0 0} {1 1}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid part count *: point and multipoint *"

test coord-6.2 {
# attempt to write multipoint with invalid number of coordinates
} -setup {
	set shp [shapefile tmp/foo multipoint {integer id 10 0}]
} -body {
	$shp coord write {{0 0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -result "2 coordinate values are expected for each vertex"

test coord-6.3 {
# write multipoint feature with one point
} -setup {
	set shp [shapefile tmp/foo multipoint {integer id 10 0}]
} -body {
	$shp coord write {{0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test coord-6.4 {
# write multipoint feature with many points
} -setup {
	set shp [shapefile tmp/foo multipoint {integer id 10 0}]
} -body {
	for {set i 0} {$i < 180} {incr i} {
		lappend vertices $i [expr {$i / 2}]
	}
	$shp coord write [list $vertices]
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test coord-6.5 {
# attempt to write multipoint feature with invalid (non-numeric) coordinates
} -setup {
	set shp [shapefile tmp/foo multipoint {integer id 10 0}]
} -body {
	$shp coord write {{foo bar}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "expected floating-point number but got *"

test coord-6.6 {
# write multipointm feature
} -setup {
	set shp [shapefile tmp/foo multipointm {integer id 10 0}]
} -body {
	$shp coord write {{0 0 1  10 10 2  88 88 3}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test coord-6.7 {
# write multipointz feature
} -setup {
	set shp [shapefile tmp/foo multipointz {integer id 10 0}]
} -body {
	$shp coord write {{0 0 0 33  1 1 1 33  2 2 2 33  3 3 3 33}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}


#
# [coord write] polygon geometry
# (polygons.test.tcl checks that polygon samples meet geometry constraints)
#

test coord-7.0 {
# zero-length or zero-area polygons are disallowed by shapefile spec, although
# parts may contain identical vertices as long as the whole part is not zero. 
# (this constraint is not currently enforced by Shapetcl; tally polygon props.)
} -constraints {
	emptyTest
}

test coord-7.1 {
# attempt to write a polygon with an empty part
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0}]
} -body {
	$shp coord write {{}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid vertex count *: polygon *"

test coord-7.2 {
# attempt to write a polygon with too few vertices
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0}]
} -body {
	$shp coord write {{0 10  10 10  0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid vertex count *: polygon *"

test coord-7.3 {
# attempt to write a polygon that is not closed (last vertex != first vertex)
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0}]
} -body {
	$shp coord write {{0 10  10 10  10 0  0 0}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid part geometry: polygon rings must be closed *"

test coord-7.3a {
# attempt to write a polygonm that is unclosed only in m dimension
} -setup {
	set shp [shapefile tmp/foo polygonm {integer id 10 0}]
} -body {
	$shp coord write {{0 10 77  10 10 77  10 0 77  0 0 77  0 10 99}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid part geometry: polygon rings must be closed *"

test coord-7.3b {
# attempt to write a polygonz that is unclosed only in z dimension
} -setup {
	set shp [shapefile tmp/foo polygonz {integer id 10 0}]
} -body {
	$shp coord write {{0 10 1 77  10 10 2 77  10 0 3 77  0 0 2 77  0 10 9 77}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid part geometry: polygon rings must be closed *"

test coord-7.3c {
# attempt to write a polygonz that is unclosed only in m dimension
} -setup {
	set shp [shapefile tmp/foo polygonz {integer id 10 0}]
} -body {
	$shp coord write {{0 10 1 77  10 10 2 77  10 0 3 77  0 0 2 77  0 10 1 99}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid part geometry: polygon rings must be closed *"

test coord-7.4 {
# use autoClosePolygon to write a polygon that is not explicitly closed
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0}]
	$shp config autoClosePolygons 1
} -body {
	$shp coord write {{0 10  10 10  10 0  0 0}}
	$shp coord read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {{0.0 10.0 10.0 10.0 10.0 0.0 0.0 0.0 0.0 10.0}}

# compare to coord-7.2 (same test without autoClosePolygons)
test coord-7.5 {
# use autoClosePolygons to complete a polygon from three open vertices
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0}]
	$shp config autoClosePolygons 1
} -body {
	$shp coord write {{0 10  10 10  0 0}}
	$shp coord read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {{0.0 10.0 10.0 10.0 0.0 0.0 0.0 10.0}}

# 3 vertices that are closed implies points A B A; adding another A means it's
# still just line segment A B, and thus zero area and an invalid polygon. We
# can catch this zero-area case because it's inescapable w/only 2 points.
test coord-7.6 {
# autoClose can't help an already-closed "polygon" of 3 vertices (a line seg)
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0}]
	$shp config autoClosePolygons 1
} -body {
	$shp coord write {{0 10  10 10  0 10}}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -result "invalid part geometry: closed ring with only 3 vertices"

test coord-7.7 {
# write an xy polygon
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0}]
} -body {
	$shp coord write {{0 10  10 10  10 0  0 0  0 10}}
	$shp coord read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {{0.0 10.0 10.0 10.0 10.0 0.0 0.0 0.0 0.0 10.0}}

test coord-7.8 {
# check that coord write rewinds backwards vertex order
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0}]
} -body {
	$shp coord write {{0 10  0 0  10 0  10 10  0 10}}
	$shp coord read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {{0.0 10.0 10.0 10.0 10.0 0.0 0.0 0.0 0.0 10.0}}

# shapelib's vertex rewinding is smart enough to "do the right thing" in 7.9/10:

test coord-7.9 {
# write a polygon with a hole in it (hole vertices are ordered counterclockwise)
} -setup {
	set shp [shapefile tmp/holes polygon {integer id 10 0}]
} -body {
	# outer ring vertex order is clockwise;
	# inner ring ("hole") vertex order is counter-clockwise
	$shp coord write {{0 10  10 10  10 0  0 0  0 10}  {3 7  3 3  7 3  7 7  3 7}}
	$shp coord read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/holes.*]
} -result {{0.0 10.0 10.0 10.0 10.0 0.0 0.0 0.0 0.0 10.0} {3.0 7.0 3.0 3.0 7.0 3.0 7.0 7.0 3.0 7.0}}

test coord-7.10 {
# write a polygon with two outer rings (islands; both part vertices clockwise)
} -setup {
	set shp [shapefile tmp/islands polygon {integer id 10 0}]
} -body {
	$shp coord write {{0 10  10 10  10 0  0 0  0 10} {13 7  17 7  17 3  13 3  13 7}}
	$shp coord read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/islands.*]
} -result {{0.0 10.0 10.0 10.0 10.0 0.0 0.0 0.0 0.0 10.0} {13.0 7.0 17.0 7.0 17.0 3.0 13.0 3.0 13.0 7.0}}

::tcltest::cleanupTests
