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

/* exit handler - invoked if shapefile is not manually closed prior to exit */
void shapefile_util_exit(ClientData clientData) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	shapefile_util_close(shapefile);
}

/* delete proc - invoked if shapefile is manually closed. deletes exit handler */
void shapefile_util_delete(ClientData clientData) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	Tcl_DeleteExitHandler(shapefile_util_exit, shapefile);
	ckfree((char *)shapefile);
}


/* set interp result to description ({type name width precision}) of an attribute table field */
int shapefile_util_fieldDescription(Tcl_Interp *interp, ShapefilePtr shapefile, int fieldi) {
	char name[12];
	int width, precision;
	DBFFieldType type;
	Tcl_Obj *description;
	
	description = Tcl_NewListObj(0, NULL);
	type = DBFGetFieldInfo(shapefile->dbf, fieldi, name, &width, &precision);
	
	switch (type) {
		case FTString:
			if (Tcl_ListObjAppendElement(interp, description, Tcl_NewStringObj("string", -1)) != TCL_OK) {
				return TCL_ERROR;
			}
			break;
		case FTInteger:
			if (Tcl_ListObjAppendElement(interp, description, Tcl_NewStringObj("integer", -1)) != TCL_OK) {
				return TCL_ERROR;
			}
			break;
		case FTDouble:
			if (Tcl_ListObjAppendElement(interp, description, Tcl_NewStringObj("double", -1)) != TCL_OK) {
				return TCL_ERROR;
			}
			break;
		default:
			/* represent unsupported field types by numeric type ID instead of descriptive name */
			if (Tcl_ListObjAppendElement(interp, description, Tcl_NewIntObj((int)type)) != TCL_OK) {
				return TCL_ERROR;
			}
			break;
	}
	
	if (Tcl_ListObjAppendElement(interp, description, Tcl_NewStringObj(name, -1)) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_ListObjAppendElement(interp, description, Tcl_NewIntObj(width)) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_ListObjAppendElement(interp, description, Tcl_NewIntObj(precision)) != TCL_OK) {
		return TCL_ERROR;
	}
	
	Tcl_SetObjResult(interp, description);
	return TCL_OK;
}

