#!/usr/bin/tclsh

lappend auto_path [file dirname [file dirname [file normalize [info script]]]]
package require Shapetcl

proc GetDimension {index format measure} {
	global in
	
	# format is a number to be assigned as explicit default
	if {[string is double $format]} {
		return $format
	}
	
	# format specifies an attribute field to read measure from
	if {[regexp {^f(\d+)$} $format match field]} {
		return [$in attributes read $index $field]
	}
	
	# format defers to implicit default (program-supplied measure; either the
	# input feature's measure or 0 if the input feature hasn't this dimension)
	if {$format eq "-"} {
		return $measure
	}
	
	# format specifies an integer range; return random double in range
	if {[regexp {^(\d+)-(\d+)$} $format match a b]} {
		set range [expr {abs($b - $a)}]
		set min [expr {min($a, $b)}]
		return [expr {$min + ($range * rand())}]
	}
	
	return 0
}

if {$argc < 2 || $argc > 4} {
	puts stderr "redim.tcl INFILE OUTFILE ?MFORMAT ?ZFORMAT??"
	puts stderr "OUTFILE will be written as a copy of INFILE with the same base type."
	puts stderr "If the MFORMAT argument is present, OUTFILE will include a Measure dimension."
	puts stderr "If the ZFORMAT argument is also present, OUTFILE will include a Z dimension."
	puts stderr "FORMATS:"
	puts stderr "  N    assign explicit default double value N for all vertices"
	puts stderr "  fN   assign vertex values from attribute field N"
	puts stderr "  A-B  assign random double value in integer range A..B"
	puts stderr "  -    assign implicit default value (existing dimension value, or 0)"
	exit 1
}
lassign $argv inFile outFile mFormat zFormat
set in [shapefile $inFile readonly]
set basetype [$in info type base]
set dimtype [$in info type dimension]
set outType $basetype
if {$zFormat ne {}} {
	append outType z
} elseif {$mFormat ne {}} {
	append outType m
}
set out [shapefile $outFile $outType [$in fields list]]

set count [$in info count]
for {set i 0} {$i < $count} {incr i} {
	set inCoords [$in coordinates read $i]
	set outCoords {}
		
	foreach inPart $inCoords {
		set outPart {}
		
		if {$dimtype eq "xyzm"} {
			# input XYZM coordinates
			foreach {x y z m} $inPart {
				lappend outPart $x $y
				if {$zFormat ne {}} {
					lappend outPart [GetDimension $i $zFormat $z]
				}
				if {$mFormat ne {}} {
					lappend outPart [GetDimension $i $mFormat $m]
				}
			}
		} elseif {$dimtype eq "xym"} {
			# input XYM coordinates
			foreach {x y m} $inPart {
				lappend outPart $x $y
				if {$zFormat ne {}} {
					lappend outPart [GetDimension $i $zFormat 0]
				}
				if {$mFormat ne {}} {
					lappend outPart [GetDimension $i $mFormat $m]
				}
			}
		} else {
			# input XY coordinates
			foreach {x y} $inPart {
				lappend outPart $x $y
				if {$zFormat ne {}} {
					lappend outPart [GetDimension $i $zFormat 0]
				}
				if {$mFormat ne {}} {
					lappend outPart [GetDimension $i $mFormat 0]
				}
			}
		}
		
		# copy Z & M from first vertex to last to ensure closed polygon rings
		if {$basetype eq "polygon"} {
			if {$zFormat ne {}} {
				lset outPart end-1 [lindex $outPart 2]
				lset outPart end [lindex $outPart 3]
			} elseif {$mFormat ne {}} {
				lset outPart end [lindex $outPart 2]
			}
		}
		
		lappend outCoords $outPart
	}	
	$out write $outCoords [$in attributes read $i]
}

$out close
$in close
