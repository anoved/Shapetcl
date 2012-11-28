#!/usr/bin/tclsh

lappend auto_path .
package require Shapetcl

if {$argc != 1} {
	puts stderr "usage: $argv0 path"
	exit 1
}

set shp [shapefile [lindex $argv 0] readonly]
set fieldCount [$shp fields count]
for {set field 0} {$field < $fieldCount} {incr field} {
	lassign [$shp field list $field] type name width precision
	puts [format "%6d: %8s %12s %4d %4d" $field $type $name $width $precision]	
}
