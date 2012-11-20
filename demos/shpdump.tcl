#!/usr/bin/tclsh

lappend auto_path .
package require Shapetcl

if {$argc != 1} {
	puts stderr "usage: $argv0 path"
	exit 1
}

set shp [shapefile [lindex $argv 0]]
puts [format "File type: %s" [$shp type]]

set fcount [$shp count]
puts [format "Features in file: %d" $fcount]

for {set fid 0} {$fid < $fcount} {incr fid} {
	set feature [$shp coords $fid -all]
	set pcount [llength $feature]
	puts "\nFeature $fid (parts: $pcount):"
	
	for {set partid 0} {$partid < $pcount} {incr partid} {
		set part [lindex $feature $partid]
		set vcount [expr {[llength $part] / 4}]
		puts " Part $partid (vertices: $vcount):"
		
		foreach {x y z m} $part {
			puts "  X: $x, Y: $y, Z: $z, M: $m"
		}
	}
}
