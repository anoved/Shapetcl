#!/usr/bin/tclsh

lappend auto_path [file dirname [file dirname [file normalize [info script]]]]
package require Shapetcl

if {$argc != 1} {
	puts stderr "usage: $argv0 path"
	exit 1
}

set shp [shapefile [lindex $argv 0]]
puts "File type: [$shp type]"

set fcount [$shp count]
puts "Features in file: $fcount"

# iterate through all features
$shp config getAllCoordinates 1
for {set fid 0} {$fid < $fcount} {incr fid} {
	set feature [$shp coordinates read $fid]
	set pcount [llength $feature]
	puts "\nFeature $fid (parts: $pcount):"
	
	# iterate through all parts of this feature
	for {set partid 0} {$partid < $pcount} {incr partid} {
		set part [lindex $feature $partid]
		set vcount [expr {[llength $part] / 4}]
		puts " Part $partid (vertices: $vcount):"
		
		# print all vertices of this feature part
		foreach {x y z m} $part {
			puts "  X: $x, Y: $y, Z: $z, M: $m"
		}
	}
}
