#include <stdio.h>
#include <string.h>
#include <tcl.h>
#include "shapefil.h"

#include "shapetcl.h"

/* return true if flagName is present in objv, otherwise false.
   for simplest parsing, expect flags at *end* of command argument list;
   if present, simply decrement objc in caller after all flags are accounted. */
int util_flagIsPresent(int objc, Tcl_Obj *CONST objv[], const char *flagName) {
	int i;
	for (i = 0; i < objc; i++) {
		if (strcmp(Tcl_GetString(objv[i]), flagName) == 0) {
			return 1;
		}
	}
	return 0;
}

/* shapefile closer - invoked if manually closed or automatically on exit */
void shapefile_util_close(ClientData clientData) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	SHPClose(shapefile->shp);
	shapefile->shp = NULL;
	DBFClose(shapefile->dbf);
	shapefile->dbf = NULL;
}

/* delete proc - invoked if shapefile is manually closed. deletes exit handler */
void shapefile_util_delete(ClientData clientData) {
	Tcl_DeleteExitHandler(shapefile_util_close, clientData);
	ckfree((char *)clientData);
}



