Miscellaneous sample data
-------------------------

This directory contains miscellaneous sample data used by the Shapetcl test suite. Note that some of these shapefiles are intentionally invalid or incomplete.

---

The `multipatch` shapefile was generated with Shapelib's `shptest` utility:

	./shptest 13
	# shptest creates test13.shp and test13.shx
	./dbfcreate test13.dbf -n id 10 0
	./dbfadd test13.dbf 0
	./dbfadd test13.dbf 1
	./dbfadd test13.dbf 2
	./dbfadd test13.dbf 3
	# renamed test13 files to multipatch

---

The `fieldnames` shapefile attribute able contains two fields with identical names, `# Field`. Shapetcl disallows the creation of duplicate field names, and also disallows the creation of field names containing non-alphanumeric_ characters or which start with non-alphabetic characters. However, these restrictions are only in place to ensure compatibility with other software which exhibits these restrictions. Shapetcl can read attribute tables whose field names do not conform to these conventions.

	./dbfcreate fieldnames.dbf -n "# Field" 10 0 -n "# Field" 19 9
