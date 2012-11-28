xym sample layers
-----------------

This directory contains sample `pointm`, `arcm`, `polygonm`, and `multipointm` shapefiles derived from the [xy sample layers](https://github.com/anoved/Shapetcl/tree/master/tests/sample/xy). The following commands were executed from the root level of the Shapetcl project directory.

Create a `pointm` layer with 0 measures based on the xy `point` layer:

	demos/redim.tcl tests/sample/xy/point tests/sample/xym/pointm 0

Create an `arcm` layer with 0 measures based on the xy `arc` layer:

	demos/redim.tcl tests/sample/xy/arc tests/sample/xym/arcm 0

Create a `polygonm` layer with 0 measures based on the xy `polygon` layer:

	demos/redim.tcl tests/sample/xy/polygon tests/sample/xym/polygonm 0

Create a `multipointm` layer with 0 measures based on the xy `multipoint` layer:

	demos/redim.tcl tests/sample/xy/multipoint tests/sample/xym/multipointm 0
