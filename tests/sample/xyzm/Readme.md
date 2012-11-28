xyzm sample layers
-----------------

This directory contains sample `pointz`, `arcz`, `polygonz`, and `multipointz` shapefiles derived from the [xym sample layers](https://github.com/anoved/Shapetcl/tree/master/tests/sample/xym). The following commands were executed from the root level of the Shapetcl project directory.

Create a `pointz` layer with random z coordinates based on the xym `pointm` layer:

	demos/redim.tcl tests/sample/xym/pointm tests/sample/xyzm/pointz - 0-100

Create an `arcz` layer with random z coordinates based on the xym `arcm` layer:

	demos/redim.tcl tests/sample/xym/arcm tests/sample/xyzm/arcz - 0-100

Create a `polygonz` layer with random z coordinates based on the xym `polygonz` layer:

	demos/redim.tcl tests/sample/xym/polygonm tests/sample/xyzm/polygonz - 0-100

Create a `multipointz` layer with random z coordinates based on the xym `multipointm` layer:

	demos/redim.tcl tests/sample/xym/multipointm tests/sample/xyzm/multipointz - 0-100
