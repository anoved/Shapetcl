#include <stdio.h>
#include <string.h>
#include <tcl.h>
#include "shapefil.h"

#include "shapetcl.h"
#include "shapetcl_info.h"

/* Local prototypes */
int shapefile_util_fieldDescription(Tcl_Interp *interp, ShapefilePtr shapefile, int fieldi);

/* mode - report shapefile access mode */
int shapefile_cmd_mode(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	}
	
	if (shapefile->readonly) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("readonly"));
	} else {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("readwrite"));
	}
	
	return TCL_OK;
}

/* count - report number of entities in shapefile (shp & dbf should match) */
int shapefile_cmd_count(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int shpCount, dbfCount;
	
	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	}
	
	SHPGetInfo(shapefile->shp, &shpCount, NULL, NULL, NULL);
	
	dbfCount = DBFGetRecordCount(shapefile->dbf);
	
	if (shpCount != dbfCount) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("shapefile feature count (%d) does not match attribute table record count (%d)", shpCount, dbfCount));
		return TCL_ERROR;
	}
	
	Tcl_SetObjResult(interp, Tcl_NewIntObj(shpCount));
	return TCL_OK;
}

/* type - report type of geometry in shapefile */
int shapefile_cmd_type(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int shpType;
	int numeric;

	if ((numeric = util_flagIsPresent(objc, objv, "-numeric"))) {
		objc--;
	}

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-numeric?");
		return TCL_ERROR;
	}
	
	SHPGetInfo(shapefile->shp, NULL, &shpType, NULL, NULL);
	
	/* in numeric mode, just return the raw code */
	if (numeric) {
		Tcl_SetObjResult(interp, Tcl_NewIntObj(shpType));
		return TCL_OK;
	}
	
	switch (shpType) {
		case SHPT_POINT:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("point"));
			break;
		case SHPT_ARC:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("arc"));
			break;
		case SHPT_POLYGON:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("polygon"));
			break;
		case SHPT_MULTIPOINT:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("multipoint"));
			break;
		
		case SHPT_POINTM:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("pointm"));
			break;
		case SHPT_ARCM:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("arcm"));
			break;
		case SHPT_POLYGONM:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("polygonm"));
			break;
		case SHPT_MULTIPOINTM:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("multipointm"));
			break;
		
		case SHPT_POINTZ:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("pointz"));
			break;
		case SHPT_ARCZ:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("arcz"));
			break;
		case SHPT_POLYGONZ:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("polygonz"));
			break;
		case SHPT_MULTIPOINTZ:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("multipointz"));
			break;
		
		default:
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("unsupported shape type (%d)", shpType));
			return TCL_ERROR;
			break;
	}
	
	return TCL_OK;
}

/* bounds - report bounds of shapefile or specified feature */
int shapefile_cmd_bounds(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int shpCount;
	double min[4], max[4];
	Tcl_Obj *bounds;
	int shpType;
	int allBounds, xyOnly;
	
	/* -all option returns all xyzm bounds even for xy or xym types */
	/* -xy option returns only xy bounds even for xym or xyzm types */
	/* -all option trumps -xy (really, only one should be present) */
	allBounds = util_flagIsPresent(objc, objv, "-all");
	xyOnly = util_flagIsPresent(objc, objv, "-xy");
	if (xyOnly) {
		objc--;
	}
	if (allBounds) {
		xyOnly = 0;
		objc--;
	}
	
	if (objc != 2 && objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?index? ?-all|-xy?");
		return TCL_ERROR;
	}
	
	/* get the file count & bounds now; we'll need the count to validate the
	   feature index, if given, in which case we'll replace min & max result. */
	SHPGetInfo(shapefile->shp, &shpCount, &shpType, min, max);
	
	if (objc == 3) {
		int featureId;
		SHPObject *obj;
		
		if (Tcl_GetIntFromObj(interp, objv[2], &featureId) != TCL_OK) {
			return TCL_ERROR;
		}
		
		if (featureId < 0 || featureId >= shpCount) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid feature index %d", featureId));
			return TCL_ERROR;
		}
		
		if ((obj = SHPReadObject(shapefile->shp, featureId)) == NULL) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to read feature %d", featureId));
			return TCL_ERROR;
		}
		
		if (obj->nSHPType == SHPT_NULL) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("no bounds for null feature"));
			SHPDestroyObject(obj);
			return TCL_ERROR;
		}
		
		min[0] = obj->dfXMin; min[1] = obj->dfYMin; min[2] = obj->dfZMin; min[3] = obj->dfMMin;
		max[0] = obj->dfXMax; max[1] = obj->dfYMax; max[2] = obj->dfZMax; max[3] = obj->dfMMax;
		
		SHPDestroyObject(obj);
	}
	
	bounds = Tcl_NewListObj(0, NULL);
	Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(min[0]));
	Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(min[1]));
	if (!xyOnly) {
		if (allBounds ||
				shpType == SHPT_POINTZ || shpType == SHPT_ARCZ ||
				shpType == SHPT_POLYGONZ || shpType == SHPT_MULTIPOINTZ) {
			Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(min[2]));
		}
		if (allBounds ||
				shpType == SHPT_POINTZ || shpType == SHPT_ARCZ ||
				shpType == SHPT_POLYGONZ || shpType == SHPT_MULTIPOINTZ ||
				shpType == SHPT_POINTM || shpType == SHPT_ARCM ||
				shpType == SHPT_POLYGONM || shpType == SHPT_MULTIPOINTM) {
			Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(min[3]));
		}
	}
	Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(max[0]));
	Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(max[1]));
	if (!xyOnly) {
		if (allBounds ||
				shpType == SHPT_POINTZ || shpType == SHPT_ARCZ ||
				shpType == SHPT_POLYGONZ || shpType == SHPT_MULTIPOINTZ) {
			Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(max[2]));
		}
		if (allBounds ||
				shpType == SHPT_POINTZ || shpType == SHPT_ARCZ ||
				shpType == SHPT_POLYGONZ || shpType == SHPT_MULTIPOINTZ ||
				shpType == SHPT_POINTM || shpType == SHPT_ARCM ||
				shpType == SHPT_POLYGONM || shpType == SHPT_MULTIPOINTM) {
			Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(max[3]));
		}	
	}
	
	Tcl_SetObjResult(interp, bounds);
	return TCL_OK;
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

/* fields - report attribute table field descriptions */
int shapefile_cmd_fields(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int fieldCount, fieldi;
	
	if (objc != 2 && objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?index|count?");
		return TCL_ERROR;
	}
	
	fieldCount = DBFGetFieldCount(shapefile->dbf);
	
	if (objc == 3) {
		
		/* check if the caller just wants the field count */
		if (strcmp(Tcl_GetString(objv[2]), "count") == 0) {
			Tcl_SetObjResult(interp, Tcl_NewIntObj(fieldCount));
			return TCL_OK;
		}
		
		/* otherwise, return description for the one specified field */
		
		if (Tcl_GetIntFromObj(interp, objv[2], &fieldi) != TCL_OK) {
			return TCL_ERROR;
		}
		
		if (fieldi < 0 || fieldi >= fieldCount) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid field index %d", fieldi));
			return TCL_ERROR;
		}
		
		/* all we need is the description for this field, so we just leave it in interp result */
		if (shapefile_util_fieldDescription(interp, shapefile, fieldi) != TCL_OK) {
			return TCL_ERROR;
		}
		
	} else {
		Tcl_Obj *descriptions;
	
		descriptions = Tcl_NewListObj(0, NULL);
		
		for (fieldi = 0; fieldi < fieldCount; fieldi++) {
			
			/* get information about this field */
			if (shapefile_util_fieldDescription(interp, shapefile, fieldi) != TCL_OK) {
				return TCL_ERROR;
			}
			
			/* append information about this field to our list of information about all fields */
			if (Tcl_ListObjAppendList(interp, descriptions, Tcl_GetObjResult(interp)) != TCL_OK) {
				return TCL_ERROR;
			}
			Tcl_ResetResult(interp);
		}
		
		Tcl_SetObjResult(interp, descriptions);
	}
	
	return TCL_OK;
}
