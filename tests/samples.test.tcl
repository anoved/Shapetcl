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

test samples-1.0 {
# open xy point
} -body {
	set shp [shapefile sample/xy/point readonly]
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {point
point
xy
243
-175.22056447761656 -41.29997393927641 179.21664709402887 64.15002361973922
} -result {}

test samples-1.1 {
# open xy arc
} -body {
	set shp [shapefile sample/xy/arc readonly]	
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {arc
arc
xy
185
-140.99778 -54.89681 141.03385176001382 70.16419
} -result {}

test samples-1.2 {
# open xy polygon
} -body {
	set shp [shapefile sample/xy/polygon readonly]	
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {polygon
polygon
xy
177
-179.99999999999997 -90.00000000000003 180.00000000000014 83.64513000000001
} -result {}

test samples-1.3 {
# open xy multipoint
} -body {
	set shp [shapefile sample/xy/multipoint readonly]	
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {multipoint
multipoint
xy
185
-140.99778 -54.89681 141.03385176001382 70.16419
} -result {}

test samples-2.0 {
# open xym pointm
} -body {
	set shp [shapefile sample/xym/pointm readonly]
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {pointm
point
xym
243
-175.22056447761656 -41.29997393927641 0.0 179.21664709402887 64.15002361973922 0.0
} -result {}

test samples-2.1 {
# open xym arcm
} -body {
	set shp [shapefile sample/xym/arcm readonly]	
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {arcm
arc
xym
185
-140.99778 -54.89681 0.0 141.03385176001382 70.16419 0.0
} -result {}


test samples-2.2 {
# open xym polygonm
} -body {
	set shp [shapefile sample/xym/polygonm readonly]	
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {polygonm
polygon
xym
177
-179.99999999999997 -90.00000000000003 0.0 180.00000000000014 83.64513000000001 0.0
} -result {}

test samples-2.3 {
# open xym multipointm
} -body {
	set shp [shapefile sample/xym/multipointm readonly]	
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {multipointm
multipoint
xym
185
-140.99778 -54.89681 0.0 141.03385176001382 70.16419 0.0
} -result {}

test samples-3.0 {
# open xyzm pointz
} -body {
	set shp [shapefile sample/xyzm/pointz readonly]
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {pointz
point
xyzm
243
-175.22056447761656 -41.29997393927641 1.465625363153231 0.0 179.21664709402887 64.15002361973922 99.64147019183332 0.0
} -result {}

test samples-3.1 {
# open xyzm arcz
} -body {
	set shp [shapefile sample/xyzm/arcz readonly]
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {arcz
arc
xyzm
185
-140.99778 -54.89681 0.017089908950538333 0.0 141.03385176001382 70.16419 99.96065646408157 0.0
} -result {}

test samples-3.2 {
# open xyzm polygonz
} -body {
	set shp [shapefile sample/xyzm/polygonz readonly]
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {polygonz
polygon
xyzm
177
-179.99999999999997 -90.00000000000003 0.008457247171763911 0.0 180.00000000000014 83.64513000000001 99.99567312188246 0.0
} -result {}

test samples-3.3 {
# open xyzm multipointz
} -body {
	set shp [shapefile sample/xyzm/multipointz readonly]
	puts [$shp info type]
	puts [$shp info type base]
	puts [$shp info type dimension]
	puts [$shp info count]
	puts [$shp info bounds]
	$shp close
} -output {multipointz
multipoint
xyzm
185
-140.99778 -54.89681 0.05532326179338771 0.0 141.03385176001382 70.16419 99.974971590552 0.0
} -result {}

::tcltest::cleanupTests
