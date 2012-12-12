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
# [info] command
#

test info-1.0 {
# invoke info cmd with too few arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp info
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test info-1.1 {
# invoke info cmd with invalid option
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp info foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "bad option *"

#
# [info count] option
#

test info-2.0 {
# invoke [info count] cmd option with too many arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp info count foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

#
# [info bounds] option
#

test info-3.0 {
# invoke [info bounds] cmd option with too many arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp info bounds 0 foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test info-3.1 {
# confirm that [info bounds] reports expected bounds (xy implied)
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp info bounds
} -cleanup {
	$shp close
} -result {-175.22056447761656 -41.29997393927641 179.21664709402887 64.15002361973922}

test info-3.1 {
# confirm that [info bounds] reports all bounds when requested despite xy type
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp config getAllCoordinates 1
	$shp info bounds
} -cleanup {
	$shp close
} -result {-175.22056447761656 -41.29997393927641 0.0 0.0 179.21664709402887 64.15002361973922 0.0 0.0}

test info-3.2 {
# confirm that [info bounds] reports xy bounds when requested for xy type
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp config getOnlyXyCoordinates 1
	$shp info bounds
} -cleanup {
	$shp close
} -result {-175.22056447761656 -41.29997393927641 179.21664709402887 64.15002361973922}

test info-3.3 {
# confirm that [info bounds] reports expected bounds (xym implied)
} -setup {
	set shp [shapefile sample/xym/pointm readonly]
} -body {
	$shp info bounds
} -cleanup {
	$shp close
} -result {-175.22056447761656 -41.29997393927641 0.0 179.21664709402887 64.15002361973922 0.0}

test info-3.4 {
# confirm that [info bounds] reports all bounds when requested for xym type
} -setup {
	set shp [shapefile sample/xym/pointm readonly]
} -body {
	$shp config getAllCoordinates 1
	$shp info bounds
} -cleanup {
	$shp close
} -result {-175.22056447761656 -41.29997393927641 0.0 0.0 179.21664709402887 64.15002361973922 0.0 0.0}

test info-3.5 {
# confirm that [info bounds] reports xy bounds when requested for xym type
} -setup {
	set shp [shapefile sample/xym/pointm readonly]
} -body {
	$shp config getOnlyXyCoordinates 1
	$shp info bounds
} -cleanup {
	$shp close
} -result {-175.22056447761656 -41.29997393927641 179.21664709402887 64.15002361973922}

test info-3.6 {
# confirm that [info bounds] reports expected bounds (xyzm implied)
} -setup {
	set shp [shapefile sample/xyzm/pointz readonly]
} -body {
	$shp info bounds
} -cleanup {
	$shp close
} -result {-175.22056447761656 -41.29997393927641 1.465625363153231 0.0 179.21664709402887 64.15002361973922 99.64147019183332 0.0}

test info-3.7 {
# confirm that [info bounds] reports all bounds for xyzm when explicitly requested
} -setup {
	set shp [shapefile sample/xyzm/pointz readonly]
} -body {
	$shp config getAllCoordinates 1
	$shp info bounds
} -cleanup {
	$shp close
} -result {-175.22056447761656 -41.29997393927641 1.465625363153231 0.0 179.21664709402887 64.15002361973922 99.64147019183332 0.0}

test info-3.8 {
# confirm that [info bounds] reports xy bounds when requested for xyzm type
} -setup {
	set shp [shapefile sample/xyzm/pointz readonly]
} -body {
	$shp config getOnlyXyCoordinates 1
	$shp info bounds
} -cleanup {
	$shp close
} -result {-175.22056447761656 -41.29997393927641 179.21664709402887 64.15002361973922}

test info-3.9 {
# invoke [info bounds index] with non-integer index
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp info bounds foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "expected integer but got *"

test info-3.10 {
# invoke [info bounds index] with index too high
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp info bounds 300
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "invalid feature index *"

test info-3.11 {
# invoke [info bounds index] with index too low
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp info bounds -1
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "invalid feature index *"

# not sure how to trigger a SHPReadObject error for extant & valid shp file

test info-3.12 {
# invoke [info bounds index] on null feature
} -setup {
	set shp [shapefile tmp/info-3-12 point {integer Id 5 0}]
	# use [attr write {record}] to add a record with a null placeholder feature;
	# normally, you'd follow up with [coord write index] to supply geometry.
	set index [$shp attributes write {0}]
} -body {
	$shp info bounds $index
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/info-3-12.*]
} -returnCodes {
	error
} -result "no bounds for null feature"

#
# [info type] option
#

test info-4.0 {
# invoke [info type] with too many arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp info type foo bar
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test info-4.1 {
# invoke [info type] with invalid type option
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp info type foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "bad option *"

test info-4.2 {
# confirm [info type] reports correct type (not testing all base/dim combi.)
} -setup {
	set shp [shapefile sample/xyzm/multipointz readonly]
} -body {
	$shp info type
} -cleanup {
	$shp close
} -result {multipointz}

# reminder that numeric type codes are defined in shapelib/shapefil.h in
# accordance with the values set forth in ESRI's Shapefile Whitepaper.
test info-4.3 {
# confirm [info type numeric] reports correct type for an arbitrary sample
} -setup {
	set shp [shapefile sample/xyzm/multipointz readonly]
} -body {
	$shp info type numeric
} -cleanup {
	$shp close
} -result {18}

test info-4.4 {
# confirm [info type base] reports correct base type for an arbitrary sample
} -setup {
	set shp [shapefile sample/xyzm/multipointz readonly]
} -body {
	$shp info type base
} -cleanup {
	$shp close
} -result {multipoint}

test info-4.5 {
# confirm [info type dimension] reports correct dimension for arbitrary sample
} -setup {
	set shp [shapefile sample/xyzm/multipointz readonly]
} -body {
	$shp info type dimension
} -cleanup {
	$shp close
} -result {xyzm} 

::tcltest::cleanupTests
