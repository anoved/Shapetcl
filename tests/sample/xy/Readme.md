xy sample layers
----------------

This directory contains sample `point`, `arc`, `polygon`, and `multipoint` shapefiles derived from [Natural Earth small-scale cultural layers](https://github.com/anoved/Shapetcl/tree/master/tests/sample/ne). The following commands were executed from the root level of the Shapetcl project directory.

Create a `point` layer as a copy of the Natural Earth populated places layer:

	demos/redim.tcl tests/sample/ne/ne_110m_populated_places_simple tests/sample/xy/point

Create an `arc` layer as a copy of the Natural Earth boundary lines layer:

	demos/redim.tcl tests/sample/ne/ne_110m_admin_0_boundary_lines_land tests/sample/xy/arc

Create a `polygon` layer as a copy of the Natural Earth countries layer:

	demos/redim.tcl tests/sample/ne/ne_110m_admin_0_countries tests/sample/xy/polygon

Create a `multipoint` layer based on the `arc` layer (same features depicted as point sets instead of lines):

	demos/multipointer.tcl tests/sample/xy/arc tests/sample/xy/multipoint
