#!/usr/bin/tclsh

lappend auto_path [file dirname [file dirname [file normalize [info script]]]]
package require shapetcl
package require http
package require json



if {$argc != 4} {
	puts stderr "usage: $argv0 LAT LON RADIUS OUTFILE"
	exit 1
}

lassign $argv lat lon radius outfile

if {![string is double $lat]} {
	puts stderr "LAT must be latitude in decimal degrees"
	exit 1
}

if {![string is double $lon]} {
	puts stderr "LON must be longitude in decimal degrees"
	exit 1
}

if {![string is double $radius]} {
	puts stderr "RADIUS must be expressed in decimal kilometers"
	exit 1
}



set query "http://api.openweathermap.org/data/2.1/find/city?lat=$lat&lon=$lon&radius=$radius"
if {[catch {::http::geturl $query} response]} {
	puts stderr $response
	exit 1
}

if {![string equal [::http::status $response] "ok"]} {
	puts stderr [::http::error $response]
	exit 1
}

set result [::json::json2dict [::http::data $response]]
set count [dict get $result "cnt"]
if {$count == 0} {
	puts stderr [format "No cities found within %s km of %s/%s" $radius $lat $lon]
	exit 1
}



set shp [::shapetcl::shapefile $outfile point {
		integer id 5 0
		string city 32 0
		double distance 19 6
		integer time 11 0
		double temp 19 6
		double windspeed 19 6
		double winddeg 19 6
		double clouds 19 6
}]



set cities [dict get $result "list"]
for {set i 0} {$i < $count} {incr i} {
	set city [lindex $cities $i]
	set coords [list [list \
			[dict get $city coord lon] \
			[dict get $city coord lat]]]
	set attrs [list \
			$i \
			[dict get $city name] \
			[dict get $city distance] \
			[dict get $city dt] \
			[expr {[dict get $city main temp] - 273.15}] \
			[dict get $city wind speed] \
			[dict get $city wind deg] \
			[dict get $city clouds all]]
	$shp write $coords $attrs
}



$shp close

