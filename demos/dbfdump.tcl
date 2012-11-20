#!/usr/bin/tclsh

lappend auto_path ..
package require Shapetcl

if {$argc != 1} {
	puts stderr "usage: $argv0 path"
	exit 1
}

set shp [shapefile [lindex $argv 0]]
set fields [$shp fields]

foreach field $fields {
	foreach {type name width precision} $field {
		puts -nonewline [format {%*s} $width $name]
	}
}
puts {}

foreach record [$shp attributes] {
	foreach value $record field $fields {
		foreach {type name width precision} $field {
			if {[string equal $type "string"]} {
				puts -nonewline [format {%*s} $width $value]
			} else {
				puts -nonewline [format {%*.*f} $width $precision $value]
			}
		}
	}
	puts {}
}
