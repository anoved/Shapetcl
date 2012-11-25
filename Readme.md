Shapetcl
========

<https://github.com/anoved/Shapetcl/> by Jim DeVona

Shapetcl is a Tcl extension that provides read-write access to shapefile coordinate and attribute data. It is based on the Shapefile C Library, otherwise known as [Shapelib](http://shapelib.maptools.org). Documentation is forthcoming; for now, see the [demo scripts](https://github.com/anoved/Shapetcl/tree/master/demos) for examples.

Limitations
-----------

Some limitations are by design, some are a result of Shapelib constraints, and some are simply to-do items.

- Shapetcl does not and will not perform any coordinate projection or transformation. Unfortunately a minimal shapefile does not specify the datum, coordinate system, projection, etc. of its coordinate data. The application developer may pre- or post-process coordinates based on any [`.prj` file](http://en.wikipedia.org/wiki/Shapefile#Shapefile_projection_format_.28.prj.29) or other metadata that may be present. The Tcllib [mapproj](http://tmml.sourceforge.net/doc/tcllib/mapproj.html) package is useful for this purpose.
- Although features may be added to a shapefile, they may not be deleted. Existing features may be changed.
- Only string, integer, and double (floating point number) attribute field types are supported.
- Multipatch features are not supported.

Related Works
-------------

Shapetcl was written to fill a particular niche I perceived to be unoccupied, but there are some similar packages you may find useful. Each of these alternatives provides a Tcl interface to Shapelib.

- [Tclshp](https://sourceforge.net/projects/tclshp/) by Devin J. Eyre. The `Tclshp` package provides limited read-write access to shapefile geometry and attribute tables. It is essentially a wrapper for some of the example code distributed with Shapelib. Each procedure call opens, accesses, and closes the shapefile, so performance may not be suitable for repeated operations. I created a simplified version as I was learning about writing Tcl extensions: [anoved/Tclshp](https://github.com/anoved/Tclshp).
- [tclshp](http://geology.usgs.gov/tools/metadata/tclshp/) and [tcldbf](http://geology.usgs.gov/tools/metadata/tcldbf/) by Peter N. Schweitzer. The `tclshp` package provides read-only access to shapefile geometry. The `tcldbf` package provides read-write access to shapefile attribute tables.
- [gpsmanshp](http://gpsmanshp.sourceforge.net) by Miguel Filgueiras. The `gpsmanshp` package provides a Tcl interface to work with higher-level GPS data using Shapelib. It is a component of [GPSMan](http://gpsman.sourceforge.net).
