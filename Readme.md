Shapetcl
========

Shapetcl is a Tcl extension that provides read-write access to shapefile coordinate and attribute data. It is based on the Shapefile C Library, otherwise known as [Shapelib](http://shapelib.maptools.org).

<https://github.com/anoved/Shapetcl/>
Written in 2012 by Jim DeVona

API and Usage Examples
----------------------

This section is incomplete. For now, here's a vague overview. Some options arguments are omitted.

The `shapefile` command opens or creates a shapefile. The return value is a token that can be used as a command to perform subsequent operations on the shapefile.

```tcl
set shp [shapefile test] ;# open test.shp/.shx/.dbf
```

Various query commands can be used to learn about the shapefile:

```tcl
$shp count   ;# number of features
$shp bounds  ;# min/max dimensions
$shp mode    ;# readonly or readwrite
$shp type    ;# point, polygon, etc.
$shp fields  ;# attribute table format
```

The `coords` command reads or overwrites coordinate geometry:

```tcl
$shp coords 0             ;# read point feature 0. features are lists
# {-75.0 40.0}               of parts; parts are lists of coordinates.
$shp coords 0 {{-80 41}}  ;# overwrite feature 0. index is returned.
# 0
$shp coords 0
# {-80.0 41.0}
```

The `attributes` command works like the `coords` command:

```tcl
$shp fields ;# gives type/name/width/precision for each field
# string NOTES 30 0 integer ID 6 0
$shp attributes 0
# {Hello, world.} 7
$shp attributes 0 {{Goodnight, moon} 21}
# 0
```

The `write` command is used to add a new feature to the shapefile. It accepts two arguments: the feature coordinates and the attribute values. It returns the index of the new feature.

```tcl
$shp write {{-76.5 42.0}} {test 0}
# 1
$shp coords 1
# {-76.5 42.0}
$shp attributes 1
# test 0
```

Last but not least, the `close` command closes the shapefiles and writes changes to disk. Any open shapefiles are automatically closed when the Tcl interpreter exits, but it's best practice to close them explicitly.

```tcl
$shp close
```

Limitations
-----------

Some limitations are by design, some are a result of Shapelib constraints, and some are simply to-do items.

- Shapetcl does not and will not perform any coordinate projection or transformation. Unfortunately a minimal shapefile does not specify the datum, coordinate system, projection, etc. of its coordinate data. The application developer may pre- or post-process coordinates based on any [`.prj` file](http://en.wikipedia.org/wiki/Shapefile#Shapefile_projection_format_.28.prj.29) or other metadata that may be present.
- Although features may be added to a shapefile, they may not be deleted. Existing features may be changed.
- Only string, integer, and double (floating point number) attribute field types are supported.
- Multipatch features are not supported.

Related Works
-------------

Shapetcl was written to fill a particular niche I perceived to be unoccupied, but there are some similar packages you may find useful. Each of these alternatives provides a Tcl interface to Shapelib.

- [Tclshp](https://sourceforge.net/projects/tclshp/) by Devin J. Eyre. The `Tclshp` package provides limited read-write access to shapefile geometry and attribute tables. It is essentially a wrapper for some of the example code distributed with Shapelib. Each procedure call opens, accesses, and closes the shapefile, so performance may not be suitable for repeated operations. I created a simplified version as I was learning about writing Tcl extensions: [anoved/Tclshp](https://github.com/anoved/Tclshp).
- [tclshp](http://geology.usgs.gov/tools/metadata/tclshp/) and [tcldbf](http://geology.usgs.gov/tools/metadata/tcldbf/) by Peter N. Schweitzer. The `tclshp` package provides read-only access to shapefile geometry. The `tcldbf` package provides read-write access to shapefile attribute tables.
- [gpsmanshp](http://gpsmanshp.sourceforge.net) by Miguel Filgueiras. The `gpsmanshp` package provides a Tcl interface to work with higher-level GPS data using Shapelib. It is a component of [GPSMan](http://gpsman.sourceforge.net).
