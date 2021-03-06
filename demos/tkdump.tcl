#!/usr/bin/tclsh

package require Tk

lappend auto_path [file dirname [file dirname [file normalize [info script]]]]
package require shapetcl
namespace import shapetcl::shapefile

if {$argc != 1} {
	puts stderr "usage: $argv0 path"
	exit 1
}

# transform presumed global geographic coords to window coords
proc Reproject {x y} {
	set px [expr {($x + 180) * 3.0}]
	set py [expr {(90 - $y) * 3.0}]
	# Mollweide Projection
	#set px [expr {540 + (180 * (2.0 * sqrt(2.0) * ($x * 0.01745329251) * cos($y * 0.01745329251)) / 3.14159265359)}]
	#set py [expr {270 - (180 * sqrt(2.0) * sin($y * 0.01745329251))}]
	return [list $px $py]
}

# list attribute values for a given feature
proc Report {shp id} {
	puts "\n--- FEATURE: $id ---"
	foreach {type name width precision} [$shp fields list] value [$shp attributes read $id] {
		puts "$name: $value"
	}
}

frame .f
canvas .f.c -xscrollcommand {.f.x set} -yscrollcommand {.f.y set} -scrollregion {0 0 1080 540} -width 1080 -height 540
scrollbar .f.x -orient horizontal -command {.f.c xview}
scrollbar .f.y -orient vertical -command {.f.c yview}
grid .f.c .f.y -sticky news
grid .f.x -sticky ew
grid rowconfigure .f 0 -weight 1
grid columnconfigure .f 0 -weight 1
pack .f -fill both -expand true

# load shapefile
set shp [shapefile [lindex $argv 0] readonly]
set basetype [$shp info type base]
set fcount [$shp info count]

# add each feature to the canvas
$shp config getOnlyXyCoordinates 1
for {set fid 0} {$fid < $fcount} {incr fid} {
	set feature [$shp coordinates read $fid]
	foreach part $feature {		
		
		# convert part to screen coordinates
		set coords {}
		foreach {x y} $part {
			lassign [Reproject $x $y] mapx mapy
			lappend coords $mapx $mapy
			if {$basetype eq "multipoint"} {
				.f.c create oval \
						[expr {$mapx - 3}] [expr {$mapy - 3}] \
						[expr {$mapx + 3}] [expr {$mapy + 3}] \
						-tags f$fid -fill black -outline {}
			}
		}
		
		# plot this part on canvas (points as circles)
		if {$basetype eq "polygon"} {
			.f.c create poly $coords -tags f$fid -fill black -outline {}
		} elseif {$basetype eq "point"} {
			.f.c create oval \
					[expr {[lindex $coords 0] - 3}] [expr {[lindex $coords 1] - 3}] \
					[expr {[lindex $coords 0] + 3}] [expr {[lindex $coords 1] + 3}] \
					-tags f$fid -fill black -outline {}
		} elseif {$basetype eq "arc"} {
			.f.c create line $coords -tags f$fid -fill black -width 4
		}
	}

	# highlight feature on hover
	.f.c bind f$fid <Enter> [list .f.c itemconfig f$fid -fill yellow]
	.f.c bind f$fid <Leave> [list .f.c itemconfig f$fid -fill black]
	
	# report attribute values on click
	.f.c bind f$fid <ButtonRelease-1> [list Report $shp $fid]
}
