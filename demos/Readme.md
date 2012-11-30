Shapetcl Demo Scripts
---------------------

This directory contains simple Tcl scripts that demonstrate portions of Shapetcl's functionality. Some of these scripts are also used to prepare the sample data used in the Shapetcl tests.

## fielddump.tcl

	fielddump.tcl SHAPEFILE

This script lists the fields in `SHAPEFILE`'s attribute table. Each line of output lists information about consecutive fields. The contents of each line are the field index (which starts at 0), the field type (`integer`, `double`, and `string` are supported), the field name, the field width, and the field precision (0 for all non-double types).

Here is an example with a shapefile named `arc` that has four fields:

	> fielddump.tcl xy/arc
	     0:  integer    scalerank   10    0
	     1:   string   featurecla   50    0
	     2:   string         name   50    0
	     3:   string     name_alt   50    0

## dbfdump.tcl

	dbfdump.tcl SHAPEFILE

This script lists the contents of `SHAPEFILE`'s attribute table. Following a header line that lists each field name, each line of output represents a consecutive attribute record.  

## shpdump.tcl

	shpdump.tcl SHAPEFILE

This script lists the coordinate geometry of each feature in `SHAPEFILE`. It first states the file type and how many features the file contains. Then, for each feature, it lists the feature index and part count (arc and polygon features may be comprised of multiple types). For each part, it then lists the part index and vertex count. Finally, all the vertex coordinates of the part are listed. Z and M ("Measure") coordinates are displayed regardless of file type and are listed as 0 by default.

	> shpdump.tcl xy/point
	File type: point
	Features in file: 243

	Feature 0 (parts: 1):
	 Part 0 (vertices: 1):
	  X: 12.453386544971766, Y: 41.903282179960115, Z: 0.0, M: 0.0

	Feature 1 (parts: 1):
	 Part 0 (vertices: 1):
	  X: 12.441770157800141, Y: 43.936095834768004, Z: 0.0, M: 0.0
	
	... 241 more features ...

## tkdump.tcl

	tkdump.tcl SHAPEFILE

This script displays the `SHAPEFILE` geometry in a Tk canvas. Vertex coordinates are assumed to be degrees latitude/longitude in the range ±90/±180 and are scaled via a crude cylindrical projection to fit the initial window dimensions. Features are highlighted in yellow when pointed at with the mouse. Clicking a feature prints its attributes to the terminal. 

[![tkdump screenshot](https://raw.github.com/anoved/Shapetcl/master/demos/tkdump-screenshot.png)](https://github.com/anoved/Shapetcl/blob/master/demos/tkdump-screenshot.png)

Note that multi-part polygons are not necessarily rendered correctly as each part is simply drawn as another polygon, whereas some may actually be "holes" instead of "islands" depending on vertex order relative to the outer ring (conventionally, the first part).

## redim.tcl

	redim.tcl INFILE OUTFILE ?MFORMAT ?ZFORMAT??

This script creates a copy of `INFILE` at `OUTFILE`, optionally modifying the feature type of `OUTFILE` to add or strip M and/or Z coordinates. If no format options are given, `OUTFILE` will be of the base type (point, arc, polygon, or multipoint) corresponding to the type of `INFILE`. If the `MFORMAT` option is given alone, `OUTFILE` will be a measured type. If the `ZFORMAT` option is also given, `OUTFILE` will include Z as well as M coordinates.

The `MFORMAT` and `ZFORMAT` options may take different forms depending how the assigned values are to be determined. If the option is simply a numeric value `N`, that value will be assigned to all vertices. If the option is of the form `fN`, then all vertices of each feature will be assigned the value of field `N` from the associated attribute table (assumed to be numeric). If the form is `A-B`, a random value between `A` and `B` (which must be integers) will be assigned to each vertex. Lastly, if the option is simply `-`, the default value will be used. If this dimension exists in `INFILE`, its values will be used; otherwise, the default is `0`.

Attribute values and X/Y coordinates are preserved.

## multipointer.tcl

	multipointer INFILE OUTFILE

This script creates a copy of `INFILE` at `OUTFILE`, converted to `multipoint` geometry type. `OUTFILE` will be of the same dimension as `INFILE` (eg M and/or Z coordinates are preserved). All vertices of each `INFILE` feature are simply aggregated into the point set of the corresponding feature in `OUTFILE`. The exception is the final vertex of polygon parts, which is omitted since it is presumed to be a duplicate of the first.