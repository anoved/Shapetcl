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



set query "http://ebird.org/ws1.1/data/obs/geo/recent?lat=$lat&lng=$lon&dist=$radius&fmt=json"
if {[catch {::http::geturl $query} response]} {
	puts stderr $response
	exit 1
}

if {![string equal [::http::status $response] "ok"]} {
	puts stderr [::http::error $response]
	exit 1
}

set results [::json::json2dict [::http::data $response]]



if {[catch {::shapetcl::shapefile $outfile point {
		integer id 5 0
		string comname 64 0
		string sciname 64 0
		string locname 64 0
		string obsdate 20 0
}} shp]} {
	puts stderr $shp
	exit 1
}



set i 0
foreach result $results {
	set coords [list [list \
			[dict get $result lng] \
			[dict get $result lat]]]
	set attrs [list \
			$i \
			[dict get $result comName] \
			[dict get $result sciName] \
			[dict get $result locName] \
			[dict get $result obsDt]]
	$shp write $coords $attrs
	incr i
}



$shp close

