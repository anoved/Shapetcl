#!/usr/bin/tclsh

lappend auto_path .
package require Shapetcl

if {$argc != 1} {
	puts stderr "usage: $argv0 path"
	exit 1
}

set shp [shapefile [lindex $argv 0]]
set fields [$shp fields]

# print header
set header {}
foreach {type name width precision} $fields {
	append header [format {%*s} $width $name]
}
puts $header

# print attribute values
foreach record [$shp attributes] {
	set line {}
	foreach value $record {type name width precision} $fields {
		if {[string equal $type "string"]} {
			append line [format {%*s} $width $value]
		} else {
			append line [format {%*.*f} $width $precision $value]
		}
	}
	puts $line
}
