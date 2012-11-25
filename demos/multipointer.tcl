#!/usr/bin/tclsh

lappend auto_path .
package require Shapetcl

if {$argc != 2} {
	puts stderr "multipointer.tcl INFILE OUTFILE"
	exit 1
}
lassign $argv inFile outFile

set in [shapefile $inFile readonly]
set count [$in count]
set inBaseType [$in type base]
set dimension [$in type dimension]
switch $dimension {
	xy {set outType "multipoint"}
	xym {set outType "multipointm"}
	xyzm {set outType "multipointz"}
}
set out [shapefile $outFile $outType [$in field list]]

for {set i 0} {$i < $count} {incr i} {
	set inCoords [$in coordinates read $i]
	set outCoords {}
	foreach inPart $inCoords {
		switch $dimension {
			xy {
				foreach {x y} $inPart {
					lappend outCoords $x $y
				}
			}
			xym {
				foreach {x y m} $inPart {
					lappend outCoords $x $y $m
				}
			}
			xyzm {
				foreach {x y z m} $inPart {
					lappend outCoords $x $y $z $m
				}
			}
		}
		# if converting from polygon, remove the last "closing" vertex
		if {$inBaseType eq "polygon"} {
			set outCoords [lreplace $outCoords end-1 end]
		}
	}
	
	$out write [list $outCoords] [$in attributes read $i]
}

$out close
$in close
