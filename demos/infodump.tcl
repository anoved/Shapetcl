#!/usr/bin/tclsh

lappend auto_path [file dirname [file dirname [file normalize [info script]]]]
package require Shapetcl

if {$argc != 1} {
	puts stderr "usage: $argv0 path"
	exit 1
}

set shp [shapefile [lindex $argv 0] readonly]
puts " Base Path: [$shp file path]"
puts "      Type: [$shp info type]"
puts " Base Type: [$shp info type base]"
puts " Dimension: [$shp info type dimension]"
puts "     Count: [$shp info count]"
puts "    Bounds: [$shp info bounds]"
