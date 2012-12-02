Shapetcl
========

Shapetcl is a Tcl extension that provides read-write access to shapefile coordinate and attribute data. It is based on the [Shapefile C Library](http://shapelib.maptools.org) ("Shapelib").

Limitations
-----------

- Shapetcl does not perform any coordinate projection. It is an application-dependent task to interpret the coordinate system of shapefile geometry data and to apply any appropriate transformation. _Tip:_ considering parsing any [`.prj` file](http://en.wikipedia.org/wiki/Shapefile#Shapefile_projection_format_.28.prj.29) that may be present and using the Tcllib [mapproj](http://tmml.sourceforge.net/doc/tcllib/mapproj.html) package to reproject coordinates accordingly.
- Although features may be added to a shapefile, they may not be deleted. Existing features may be changed.
- Although fields may be added to a shapefile attribute table, they may not be deleted or otherwise modified.
- Only string, integer, and double (floating point number) attribute field types are supported.
- Multipatch features are not supported.

Related Works
-------------

- [Tclshp](https://sourceforge.net/projects/tclshp/) by Devin J. Eyre. The `Tclshp` package provides limited read-write access to shapefile geometry and attribute tables. It is a wrapper for some of the example programs distributed with Shapelib. Each procedure call opens, accesses, and closes a shapefile, so performance is not suitable for repeated operations. I created a simplified version as I was learning about writing Tcl extensions: [anoved/Tclshp](https://github.com/anoved/Tclshp).
- [tclshp](http://geology.usgs.gov/tools/metadata/tclshp/) and [tcldbf](http://geology.usgs.gov/tools/metadata/tcldbf/) by Peter N. Schweitzer. The `tclshp` package provides read-only access to shapefile geometry. The `tcldbf` package provides read-write access to shapefile attribute tables.
- [gpsmanshp](http://gpsmanshp.sourceforge.net) by Miguel Filgueiras. The `gpsmanshp` package provides a Tcl interface to work with higher-level GPS data using Shapelib. It is a component of [GPSMan](http://gpsman.sourceforge.net).
- [tcl-map](http://code.google.com/p/tcl-map/) by Alexandros Stergiakis. A 2009 "Google Summer of Code" project to improve Tcl's GIS support, particularly with GDAL/OGR bindings.

License
-------

Shapetcl is freely distributed under an open source [MIT License](http://opensource.org/licenses/MIT):

> Copyright (c) 2012 Jim DeVona
>
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
