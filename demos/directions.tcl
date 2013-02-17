#!/usr/bin/tclsh

lappend auto_path [file dirname [file dirname [file normalize [info script]]]]
package require shapetcl
package require http
package require json

if {$argc != 3} {
	puts stderr "usage: $argv0 FROMLOCATION TOLOCATION OUTFILE"
	exit 1
}

lassign $argv from to outfile

# http://open.mapquestapi.com/directions/
::http::config -useragent "Shapetcl directions demo (https://github.com/anoved/Shapetcl)"
set query [::http::formatQuery from $from to $to shapeFormat raw generalize 0]
if {[catch {::http::geturl "http://open.mapquestapi.com/directions/v0/route?$query"} response]} {
	puts stderr $response
	exit 1
}

if {[::http::status $response] ne "ok"} {
	puts stderr [::http::error $response]
	exit 1
}

set results [::json::json2dict [::http::data $response]]

if {[catch {::shapetcl::shapefile $outfile arc {
		integer id 11 0
		double distance 19 6
		string direction 10 0
		string time 10 0
		string narrative 128 0
}} shp]} {
	puts stderr $shp
	exit 1
}
$shp config allowTruncation 1

# get the point list, with coordinates swapped to x-y sequence
foreach {y x} [dict get $results route shape shapePoints] {lappend points $x $y}

# the mapping list maps maneuver to points int he coordinate list
set mapping [dict get $results route shape maneuverIndexes]
set mcount [llength $mapping]

# output an attributed line segment for each "maneuver";
# there is only one leg in a route between two locations.
set leg [lindex [dict get $results route legs] 0]
foreach maneuver [dict get $leg maneuvers] {
	
	# the last "maneuver" just marks the endpoint
	set id [dict get $maneuver index]
	if {$id + 1 == $mcount} {break}
		
	# pull the coordinates for this segment from the points list
	set firstPoint [lindex $mapping $id]
	set lastPoint  [lindex $mapping [expr {$id + 1}]]
	set coords [list [lrange $points \
			[expr {$firstPoint * 2}] \
			[expr {($lastPoint * 2) - 1}]]]
	
	# assemble the attributes list for this segment
	set attrs [list $id \
			[dict get $maneuver distance] \
			[dict get $maneuver directionName] \
			[dict get $maneuver formattedTime] \
			[dict get $maneuver narrative]]
	
	$shp write $coords $attrs
}

$shp close
