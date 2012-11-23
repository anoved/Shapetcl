#include <stdio.h>
#include <string.h>
#include "shapefil.h"
#include <tcl.h>

typedef struct {
	SHPHandle shp;
	DBFHandle dbf;
	int readonly;
} shapetcl_shapefile;
typedef shapetcl_shapefile * ShapefilePtr;

static int COMMAND_COUNT = 0;

int shapefile_util_attrWrite(Tcl_Interp *interp, ShapefilePtr shapefile, int recordId, int validate, Tcl_Obj *attrList);

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

/* close - flush shapefile and delete associated command */
int shapefile_cmd_close(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	}
	
	shapefile_util_close(shapefile);
	
	/* triggers the deleteProc associated with this shapefile cmd: shapefile_util_delete */
	Tcl_DeleteCommand(interp, Tcl_GetString(objv[0]));
	
	return TCL_OK;
}

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

int shapefile_util_coordWrite(Tcl_Interp *interp, ShapefilePtr shapefile, int featureId, Tcl_Obj *coordParts) {
	int shapeType, featureCount;
	int outputFeatureId;
	int *partStarts;
	Tcl_Obj *coords, *coord;
	int part, partCount, partCoord, partCoordCount;
	int vertex, vertexCount, partVertexCount;
	double *xCoords, *yCoords, *zCoords, *mCoords, x, y, z, m;
	SHPObject *shape;
	int returnValue = TCL_OK;
	int addClosingVertex = 0;
	int coordinatesPerVertex;
	
	if (shapefile->readonly) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("cannot write coords to readonly shapefile"));
		return TCL_ERROR;
	}
	
	SHPGetInfo(shapefile->shp, &featureCount, &shapeType, NULL, NULL);
	if (featureId < -1 || featureId >= featureCount) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid feature index %d", featureId));
		return TCL_ERROR;
	}
	/* a featureId of -1 indicates a new feature should be output */
	
	if (Tcl_ListObjLength(interp, coordParts, &partCount) != TCL_OK) {
		return TCL_ERROR;
	}
	
	/* validate feature by number of parts according to shape type */
	if ((shapeType == SHPT_POINT || shapeType == SHPT_POINTM || shapeType == SHPT_POINTZ) &&
			partCount != 1) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid part count (%d): point features must have exactly 1 part", partCount));
		return TCL_ERROR;
	}
	if ((shapeType == SHPT_MULTIPOINT || shapeType == SHPT_ARC || shapeType == SHPT_POLYGON ||
			shapeType == SHPT_MULTIPOINTM || shapeType == SHPT_ARCM || shapeType == SHPT_POLYGONM ||
			shapeType == SHPT_MULTIPOINTZ || shapeType == SHPT_ARCZ || shapeType == SHPT_POLYGONZ) &&
			partCount < 1) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid part count (%d): arc, polygon, and multipoint features must have at least 1 part", partCount));
		return TCL_ERROR;
	}
	
	/* determine how many coordinates to expect for each vertex */
	if (shapeType == SHPT_POINTZ || shapeType == SHPT_ARCZ || shapeType == SHPT_POLYGONZ || shapeType == SHPT_MULTIPOINTZ) {
		/* indicates xyzm type */
		coordinatesPerVertex = 4;
	} else if (shapeType == SHPT_POINTM || shapeType == SHPT_ARCM || shapeType == SHPT_POLYGONM || shapeType == SHPT_MULTIPOINTM) {
		/* indicates xym type */
		coordinatesPerVertex = 3;
	} else {
		/* indicates xy type */
		coordinatesPerVertex = 2;
	}
		
	if ((partStarts = (int *)ckalloc(sizeof(int) * partCount)) == NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to allocate coord part index array"));
		return TCL_ERROR;
	}
	xCoords = NULL; yCoords = NULL; zCoords = NULL; mCoords = NULL;

	vertex = 0;
	vertexCount = 0;
	
	for (part = 0; part < partCount; part++) {
		partStarts[part] = vertex;
		
		/* get the coordinates that comprise this part */
		if (Tcl_ListObjIndex(interp, coordParts, part, &coords) != TCL_OK) {
			returnValue = TCL_ERROR;
			goto cwRelease;
		}
		
		/* verify that the coordinate list has a valid number of elements */
		if (Tcl_ListObjLength(interp, coords, &partCoordCount) != TCL_OK) {
			returnValue = TCL_ERROR;
			goto cwRelease;
		}
		if (partCoordCount % coordinatesPerVertex != 0) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("%d coordinate values are expected for each vertex", coordinatesPerVertex));
			returnValue = TCL_ERROR;
			goto cwRelease;
		}
		partVertexCount = partCoordCount / coordinatesPerVertex;
		
		/* validate part by number of vertices according to shape type */
		if ((shapeType == SHPT_POINT || shapeType == SHPT_POINTM || shapeType == SHPT_POINTZ ||
				shapeType == SHPT_MULTIPOINT || shapeType == SHPT_MULTIPOINTM || shapeType == SHPT_MULTIPOINTZ) &&
				partVertexCount != 1) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid vertex count (%d): point and multipoint features must have exactly one vertex per part", partVertexCount));
			returnValue = TCL_ERROR;
			goto cwRelease;
		}
		if ((shapeType == SHPT_ARC || shapeType == SHPT_ARCM || shapeType == SHPT_ARCZ) && partVertexCount < 2) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid vertex count (%d): arc features must have at least 2 vertices per part", partVertexCount));
			returnValue = TCL_ERROR;
			goto cwRelease;
		}
		if ((shapeType == SHPT_POLYGON || shapeType == SHPT_POLYGONM || shapeType == SHPT_POLYGONZ) && partVertexCount < 4) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid vertex count (%d): polygon features must have at least 4 vertices per part", partVertexCount));
			returnValue = TCL_ERROR;
			goto cwRelease;
		}
				
		/* add space for this part's vertices */
		vertexCount += partVertexCount;
		xCoords = (double *)ckrealloc((char *)xCoords, sizeof(double) * vertexCount);
		yCoords = (double *)ckrealloc((char *)yCoords, sizeof(double) * vertexCount);
		if (coordinatesPerVertex == 4) {
			zCoords = (double *)ckrealloc((char *)zCoords, sizeof(double) * vertexCount);
		}
		if (coordinatesPerVertex == 4 || coordinatesPerVertex == 3) {	
			mCoords = (double *)ckrealloc((char *)mCoords, sizeof(double) * vertexCount);
		}
		if (xCoords == NULL || yCoords == NULL ||
				(coordinatesPerVertex == 4 && zCoords == NULL) ||
				((coordinatesPerVertex == 4 || coordinatesPerVertex == 3) && mCoords == NULL)) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to reallocate memory for coordinate arrays"));
			returnValue = TCL_ERROR;
			goto cwRelease;
		}
		
		for (partCoord = 0; partCoord < partCoordCount; partCoord += coordinatesPerVertex) {
			
			/* x */
			if (Tcl_ListObjIndex(interp, coords, partCoord, &coord) != TCL_OK) {
				returnValue = TCL_ERROR;
				goto cwRelease;
			}
			if (Tcl_GetDoubleFromObj(interp, coord, &x) != TCL_OK) {
				returnValue = TCL_ERROR;
				goto cwRelease;
			}
			xCoords[vertex] = x;
			
			/* y */
			if (Tcl_ListObjIndex(interp, coords, partCoord + 1, &coord) != TCL_OK) {
				returnValue = TCL_ERROR;
				goto cwRelease;
			}
			if (Tcl_GetDoubleFromObj(interp, coord, &y) != TCL_OK) {
				returnValue = TCL_ERROR;
				goto cwRelease;
			}
			yCoords[vertex] = y;
			
			/* z & m */
			if (coordinatesPerVertex == 4) {
				if (Tcl_ListObjIndex(interp, coords, partCoord + 2, &coord) != TCL_OK) {
					returnValue = TCL_ERROR;
					goto cwRelease;
				}
				if (Tcl_GetDoubleFromObj(interp, coord, &z) != TCL_OK) {
					returnValue = TCL_ERROR;
					goto cwRelease;
				}
				zCoords[vertex] = z;
				
				if (Tcl_ListObjIndex(interp, coords, partCoord + 3, &coord) != TCL_OK) {
					returnValue = TCL_ERROR;
					goto cwRelease;
				}
				if (Tcl_GetDoubleFromObj(interp, coord, &m) != TCL_OK) {
					returnValue = TCL_ERROR;
					goto cwRelease;
				}
				mCoords[vertex] = m;
			}
			
			/* m only */
			if (coordinatesPerVertex == 3) {
				if (Tcl_ListObjIndex(interp, coords, partCoord + 2, &coord) != TCL_OK) {
					returnValue = TCL_ERROR;
					goto cwRelease;
				}
				if (Tcl_GetDoubleFromObj(interp, coord, &m) != TCL_OK) {
					returnValue = TCL_ERROR;
					goto cwRelease;
				}
				mCoords[vertex] = m;
			}
			
			vertex++;
		}

		/* validate that the first and last vertex of polygon parts match */
		/* M coordinates do not have to match, but Z coord of POLYGONZ must */
		if ((shapeType == SHPT_POLYGON || shapeType == SHPT_POLYGONM || shapeType == SHPT_POLYGONZ) &&
				((xCoords[partStarts[part]] != xCoords[vertex-1]) ||
				 (yCoords[partStarts[part]] != yCoords[vertex-1]) ||
				 (coordinatesPerVertex == 4 && (zCoords[partStarts[part]] != zCoords[vertex-1])))) {
			if (addClosingVertex) {
				/* close the part automatically by appending the first vertex */
				partVertexCount++;
				vertexCount++;
				xCoords = (double *)ckrealloc((char *)xCoords, sizeof(double) * vertexCount);
				yCoords = (double *)ckrealloc((char *)yCoords, sizeof(double) * vertexCount);
				if (coordinatesPerVertex == 4) {
					zCoords = (double *)ckrealloc((char *)zCoords, sizeof(double) * vertexCount);
				}
				if (coordinatesPerVertex == 4 || coordinatesPerVertex == 3) {
					mCoords = (double *)ckrealloc((char *)mCoords, sizeof(double) * vertexCount);
				}
				if (xCoords == NULL || yCoords == NULL ||
						(coordinatesPerVertex == 4 && zCoords == NULL) ||
						((coordinatesPerVertex == 4 || coordinatesPerVertex == 3) && mCoords == NULL)) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to reallocate coordinate arrays for closing vertex"));
					returnValue = TCL_ERROR;
					goto cwRelease;
				}
				xCoords[vertex] = xCoords[partStarts[part]];
				yCoords[vertex] = yCoords[partStarts[part]];
				if (coordinatesPerVertex == 4) {
					zCoords[vertex] = zCoords[partStarts[part]];
				}
				if (coordinatesPerVertex == 4 || coordinatesPerVertex == 3) {
					mCoords[vertex] = mCoords[partStarts[part]];
				}
				vertex++;
			} else {
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid part geometry: polygon rings must be closed (begin and end with the same vertex)"));
				returnValue = TCL_ERROR;
				goto cwRelease;
			}
		}
	}
	
	/* assemble the coordinate lists into a new shape (z & m may be NULL) */
	if ((shape = SHPCreateObject(shapeType, featureId, partCount,
			partStarts, NULL, vertexCount, xCoords, yCoords, zCoords, mCoords)) == NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to create shape object"));
		returnValue = TCL_ERROR;
		goto cwRelease;
	}
	
	/* correct the shape's vertex order, if necessary */
	SHPRewindObject(shapefile->shp, shape);
	
	/* write the shape to the shapefile */
	if ((outputFeatureId = SHPWriteObject(shapefile->shp, featureId, shape)) == -1) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to write shape object"));
		returnValue = TCL_ERROR;
		goto cwDestroyRelease;
	}

	Tcl_SetObjResult(interp, Tcl_NewIntObj(outputFeatureId));
	
   cwDestroyRelease:
	SHPDestroyObject(shape);
	
   cwRelease:
	if (partStarts != NULL) ckfree((char *)partStarts);
	if (xCoords != NULL) ckfree((char *)xCoords);
	if (yCoords != NULL) ckfree((char *)yCoords);
	if (zCoords != NULL) ckfree((char *)zCoords);
	if (mCoords != NULL) ckfree((char *)mCoords);
	
	return returnValue;
}

int shapefile_util_coordRead(Tcl_Interp *interp, ShapefilePtr shapefile, int featureId, int allCoords, int xyOnly) {
	SHPObject *shape;
	Tcl_Obj *coordParts;
	int featureCount, part, partCount, vertex, vertexStart, vertexStop;
	int returnValue = TCL_OK;
	int shpType;
	
	SHPGetInfo(shapefile->shp, &featureCount, &shpType, NULL, NULL);
	if (featureId < 0 || featureId >= featureCount) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid feature index %d", featureId));
		return TCL_ERROR;
	}
	
	if ((shape = SHPReadObject(shapefile->shp, featureId)) == NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to read feature %d", featureId));
		return TCL_ERROR;
	}
	
	/* prepare a list of coordinate lists; each member list represents one part
	   (a "ring") of the feature. Some features only have one part. */
	coordParts = Tcl_NewListObj(0, NULL);
	
	/* initialize vertex indices. if there's only one part, add all vertices;
	   if there are multiple parts, stop adding vertices at 1st part boundary */
	part = 0;
	vertexStart = 0;
	if (shape->nParts < 2) {
		partCount = 1;
		vertexStop = shape->nVertices;
	} else {
		partCount = shape->nParts;
		vertexStop = shape->panPartStart[1];
	}
	
	while (part < partCount) {
		
		/* prepare a coordinate list for this part */
		Tcl_Obj *coords = Tcl_NewListObj(0, NULL);
		
		/* get the vertex coordinates for this part */
		for (vertex = vertexStart; vertex < vertexStop; vertex++) {
			if (Tcl_ListObjAppendElement(interp, coords, Tcl_NewDoubleObj(shape->padfX[vertex])) != TCL_OK) {
				returnValue = TCL_ERROR;
				goto crRelease;
			}
			if (Tcl_ListObjAppendElement(interp, coords, Tcl_NewDoubleObj(shape->padfY[vertex])) != TCL_OK) {
				returnValue = TCL_ERROR;
				goto crRelease;
			}
			
			/* don't even bother considering Z or M coords if only xy requested */
			if (xyOnly) {
				continue;
			}
			
			/* for Z type features, append Z coordinate before Measure */
			if (allCoords ||
					shpType == SHPT_POINTZ || shpType == SHPT_ARCZ ||
					shpType == SHPT_POLYGONZ || shpType == SHPT_MULTIPOINTZ) {
				
				/* append Z coordinate */
				if (Tcl_ListObjAppendElement(interp, coords, Tcl_NewDoubleObj(shape->padfZ[vertex])) != TCL_OK) {
					returnValue = TCL_ERROR;
					goto crRelease;
				}
			}
			
			/* for M and Z type features, append Measure (if used) last */
			if (allCoords ||
					shpType == SHPT_POINTZ || shpType == SHPT_ARCZ ||
					shpType == SHPT_POLYGONZ || shpType == SHPT_MULTIPOINTZ ||
					shpType == SHPT_POINTM || shpType == SHPT_ARCM ||
					shpType == SHPT_POLYGONM || shpType == SHPT_MULTIPOINTM) {
				
				/* append M coordinate, or 0.0 if unused despite type */
				if (Tcl_ListObjAppendElement(interp, coords,
						Tcl_NewDoubleObj(shape->bMeasureIsUsed ? shape->padfM[vertex] : 0.0)) != TCL_OK) {
					returnValue = TCL_ERROR;
					goto crRelease;
				}
			}			
		}
		
		/* add this part's coordinate list to the feature's part list */
		if (Tcl_ListObjAppendElement(interp, coordParts, coords) != TCL_OK) {
			returnValue = TCL_ERROR;
			goto crRelease;
		}
		
		/* advance vertex indices to the next part (disregarded if none) */
		vertexStart = vertex;
		if (part + 2 < partCount) {
			vertexStop = shape->panPartStart[part + 2];
		} else {
			vertexStop = shape->nVertices;
		}
		part++;
	}
	
	Tcl_SetObjResult(interp, coordParts);

   crRelease:
	SHPDestroyObject(shape);	
	return returnValue;
}

/* coords - get or set feature coordinates */
int shapefile_cmd_coords(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int featureId;
	int opt_allCoords, opt_xyOnly;
	
	static const char *subcommandNames[] = {"read", "write", NULL};
	int subcommandIndex;
	
	opt_allCoords = util_flagIsPresent(objc, objv, "-all");
	opt_xyOnly = util_flagIsPresent(objc, objv, "-xy");
	if (opt_xyOnly) {
		objc--;
	}
	if (opt_allCoords) {
		opt_xyOnly = 0;
		objc--;
	}
	
	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "action ?args?");
		return TCL_ERROR;
	}
	if (Tcl_GetIndexFromObj(interp, objv[2], subcommandNames, "action",
			TCL_EXACT, &subcommandIndex) != TCL_OK) {
		return TCL_ERROR;
	}
	
	if (subcommandIndex == 0) {
		/* read coords */
		
		if (objc == 3) {
			/* return coords of all features */
			Tcl_Obj *featureList;
			int shpCount;
			
			featureList = Tcl_NewListObj(0, NULL);
			SHPGetInfo(shapefile->shp, &shpCount, NULL, NULL, NULL);
			
			for (featureId = 0; featureId < shpCount; featureId++) {
				
				if (shapefile_util_coordRead(interp, shapefile, featureId, opt_allCoords, opt_xyOnly) != TCL_OK) {
					return TCL_ERROR;
				}
				
				if (Tcl_ListObjAppendElement(interp, featureList, Tcl_GetObjResult(interp)) != TCL_OK) {
					return TCL_ERROR;
				}
				
				Tcl_ResetResult(interp);
			}
			
			Tcl_SetObjResult(interp, featureList);			
		} else if (objc == 4) {
		
			/* get feature index to read */			
			if (Tcl_GetIntFromObj(interp, objv[3], &featureId) != TCL_OK) {
				return TCL_ERROR;
			}
			
			/* return coords of specified feature index */
			if (shapefile_util_coordRead(interp, shapefile, featureId, opt_allCoords, opt_xyOnly) != TCL_OK) {
				return TCL_ERROR;
			}
			
		} else {
			Tcl_WrongNumArgs(interp, 3, objv, "?index? ?-all|-xy?");
			return TCL_ERROR;
		}
	} else if (subcommandIndex == 1) {
		/* write coords */
		
		if (objc == 4) {
			/* write coords to a new feature; create complementary blank attribute record */
			int recordId;
			
			/* write coords to a new feature */
			if (shapefile_util_coordWrite(interp, shapefile, -1, objv[3]) != TCL_OK) {
				return TCL_ERROR;
			}
			
			Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp), &featureId);
			Tcl_ResetResult(interp);
			
			/* interp result is new feature id; create a null attribute record to match */
			if (shapefile_util_attrWrite(interp, shapefile, -1, 0, NULL) != TCL_OK) {
				return TCL_ERROR;
			}
			
			Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp), &recordId);
			if (featureId != recordId) {
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("new feature index (%d) does not match new empty attribute record index (%d)", featureId, recordId));
				return TCL_ERROR;
			}
				
		} else if (objc == 5) {
			/* write coords to a specific feature index */
			
			/* get feature index to overwrite */
			if (Tcl_GetIntFromObj(interp, objv[3], &featureId) != TCL_OK) {
				return TCL_ERROR;
			}
			
			/* if shape output is successful, interp result is set to output feature id */
			if (shapefile_util_coordWrite(interp, shapefile, featureId, objv[4]) != TCL_OK) {
				return TCL_ERROR;
			}
		} else {
			Tcl_WrongNumArgs(interp, 3, objv, "?index? coords");
			return TCL_ERROR;
		}
	}
		
	return TCL_OK;
}

/* given a list of attributes, check whether they conform to the shapefile's
   field specification (parseable by type and not truncated). If not, return
   TCL_ERROR with reason in result. If so, return TCL_OK with attribute list. */
int shapefile_util_attrValidate(Tcl_Interp *interp, ShapefilePtr shapefile, Tcl_Obj *attrList) {
	int fieldi, fieldCount, attrCount;
	DBFFieldType fieldType;
	int width, precision;
	Tcl_Obj *attr;
	char numericStringValue[64];
	int intValue;
	double doubleValue;
	const char *stringValue;
	
	fieldCount = DBFGetFieldCount(shapefile->dbf);
	if (Tcl_ListObjLength(interp, attrList, &attrCount) != TCL_OK) {
		return TCL_ERROR;
	}
	if (attrCount != fieldCount) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("attribute count (%d) does not match field count (%d)", attrCount, fieldCount));
		return TCL_ERROR;
	}
	
	/* validation pass - confirm field values can be parsed as field type
	   (and perhaps check for obvious cases of truncation) before output */
	for (fieldi = 0; fieldi < fieldCount; fieldi++) {
		
		/* grab the attr provided for this field */
		if (Tcl_ListObjIndex(interp, attrList, fieldi, &attr) != TCL_OK) {
			return TCL_ERROR;
		}
		
		fieldType = DBFGetFieldInfo(shapefile->dbf, fieldi, NULL, &width, &precision);

		/* if it is an empty string {}, we'll write it as a NULL value
		   regardless of field type */
		if (Tcl_GetCharLength(attr) == 0) {
			continue;
		}
				
		switch (fieldType) {
			case FTInteger:
				
				/* can this value be parsed as an integer? */
				if (Tcl_GetIntFromObj(interp, attr, &intValue) != TCL_OK) {
					return TCL_ERROR;
				}
				
				/* does this integer fit within the field width? */
				sprintf(numericStringValue, "%d", intValue);
				if (strlen(numericStringValue) > width) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("integer value (%s) would be truncated to field width (%d)", numericStringValue, width));
					return TCL_ERROR;
				}
				break;
			case FTDouble:
				
				/* can this value be parsed as a double? */
				if (Tcl_GetDoubleFromObj(interp, attr, &doubleValue) != TCL_OK) {
					return TCL_ERROR;
				}
				
				/* does this double fit within the field width? */
				sprintf(numericStringValue, "%.*lf", precision, doubleValue);
				if (strlen(numericStringValue) > width) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("double value (%s) would be truncated to field width (%d)", numericStringValue, width));
					return TCL_ERROR;
				}
				break;
			case FTString:
			
				/* can this value be parsed as a string? */
				if ((stringValue = Tcl_GetString(attr)) == NULL) {
					return TCL_ERROR;
				}
				
				/* does this string fit within the field width? */
				if (strlen(stringValue) > width) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("string value (%s) would be truncated to field width (%d)", stringValue, width));
					return TCL_ERROR;
				}
				break;
			default:
				/* NULL values will be written for unsupported field types */
				continue;
				break;
		}
	}
	
	Tcl_SetObjResult(interp, attrList);
	return TCL_OK;
}

int shapefile_util_attrWrite(Tcl_Interp *interp, ShapefilePtr shapefile, int recordId, int validate, Tcl_Obj *attrList) {
	Tcl_Obj *attr;
	int fieldi, fieldCount, attrCount, dbfCount;
	DBFFieldType fieldType;
	int intValue;
	double doubleValue;
	const char *stringValue;
	
	if (shapefile->readonly) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("cannot write attributes to readonly shapefile"));
		return TCL_ERROR;
	}
	
	dbfCount = DBFGetRecordCount(shapefile->dbf);
	if (recordId < -1 || recordId >= dbfCount) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid record index %d", recordId));
		return TCL_ERROR;
	}
	if (recordId == -1) {
		recordId = dbfCount;
	}
	
	/* now recordId is either the id of an existing record to overwrite,
	   or the id-elect of a new record we will create. Unlike shape output,
	   we use predicted id instead of -1 because each field value must be
	   written by a separate DBF*Writer function call, and we don't want
	   to write each value to a different new record! */
	
	/* verify the provided attribute value list matches field count */
	fieldCount = DBFGetFieldCount(shapefile->dbf);
	
	/* as a special case, simply write null values for all fields if attrList is NULL */
	if (attrList == NULL) {
		for (fieldi = 0; fieldi < fieldCount; fieldi++) {
			if (DBFWriteNULLAttribute(shapefile->dbf, recordId, fieldi) == 0) {
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to write null attribute"));
				return TCL_ERROR;
			}
		}
		Tcl_SetObjResult(interp, Tcl_NewIntObj(recordId));
		return TCL_OK;
	}
	
	if (Tcl_ListObjLength(interp, attrList, &attrCount) != TCL_OK) {
		return TCL_ERROR;
	}
	if (attrCount != fieldCount) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("attribute count (%d) does not match field count (%d)", attrCount, fieldCount));
		return TCL_ERROR;
	}
	
	/* in the attributes command context, we validate now; w/the write command,
	   we receive attrList pre-validated and can proceed to write it as-is. */
	if (validate && (shapefile_util_attrValidate(interp, shapefile, attrList) != TCL_OK)) {
		return TCL_ERROR;
	}
		
	/* output pass - once the fields are validated, write 'em out. Output is
	   performed separately from validation to avoid mangled/partial output. */
	for (fieldi = 0; fieldi < fieldCount; fieldi++) {
		
		/* grab the attr provided for this field */
		if (Tcl_ListObjIndex(interp, attrList, fieldi, &attr) != TCL_OK) {
			return TCL_ERROR;
		}

		fieldType = DBFGetFieldInfo(shapefile->dbf, fieldi, NULL, NULL, NULL);
	
		/* null value */
		if (Tcl_GetCharLength(attr) == 0) {
			if (!DBFWriteNULLAttribute(shapefile->dbf, recordId, fieldi)) {
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to write null attribute"));
				return TCL_ERROR;
			}
			continue;
		}
		
		/* regular values */
		switch (fieldType) {
			case FTInteger:
				if ((Tcl_GetIntFromObj(interp, attr, &intValue) != TCL_OK) ||
						(!DBFWriteIntegerAttribute(shapefile->dbf, recordId, fieldi, intValue))) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to write integer attribute \"%d\"", intValue));
					return TCL_ERROR;
				}
				break;
			case FTDouble:
				if ((Tcl_GetDoubleFromObj(interp, attr, &doubleValue) != TCL_OK) ||
						(!DBFWriteDoubleAttribute(shapefile->dbf, recordId, fieldi, doubleValue))) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to write double attribute \"%lf\"", doubleValue));
					return TCL_ERROR;
				}
				break;
			case FTString:
				if (((stringValue = Tcl_GetString(attr)) == NULL) ||
						(!DBFWriteStringAttribute(shapefile->dbf, recordId, fieldi, stringValue))) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to write string attribute \"%s\"", stringValue));
					return TCL_ERROR;
				}
				break;
			default:
				/* write NULL for all unsupported field types */
				if (!DBFWriteNULLAttribute(shapefile->dbf, recordId, fieldi)) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to write null attribute for unsupported field %d", fieldi));
					return TCL_ERROR;
				}
				break;
		}
	}
		
	Tcl_SetObjResult(interp, Tcl_NewIntObj(recordId));
	return TCL_OK;
}

int shapefile_util_attrRead(Tcl_Interp *interp, ShapefilePtr shapefile, int recordId) {
	Tcl_Obj *attributes = Tcl_NewListObj(0, NULL);
	int fieldi, fieldCount, dbfCount, fieldType;
	
	fieldCount  = DBFGetFieldCount(shapefile->dbf);
	dbfCount = DBFGetRecordCount(shapefile->dbf);
	if (recordId < 0 || recordId >= dbfCount) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid record index %d", recordId));
		return TCL_ERROR;
	}

	for (fieldi = 0; fieldi < fieldCount; fieldi++) {
				
		/* represent NULL attribute values as {} in return list */
		if (DBFIsAttributeNULL(shapefile->dbf, recordId, fieldi)) {
			if (Tcl_ListObjAppendElement(interp, attributes, Tcl_NewObj()) != TCL_OK) {
				return TCL_ERROR;
			}
			continue;
		}
		
		/* interpret attribute value according to field type and append to result list */
		fieldType = (int)DBFGetFieldInfo(shapefile->dbf, fieldi, NULL, NULL, NULL);
		switch (fieldType) {
			case FTInteger:
				if (Tcl_ListObjAppendElement(interp, attributes,
						Tcl_NewIntObj(DBFReadIntegerAttribute(shapefile->dbf, recordId, fieldi))) != TCL_OK) {
					return TCL_ERROR;				
				}
				break;
			case FTDouble:
				if (Tcl_ListObjAppendElement(interp, attributes,
						Tcl_NewDoubleObj(DBFReadDoubleAttribute(shapefile->dbf, recordId, fieldi))) != TCL_OK) {
					return TCL_ERROR;
				}
				break;
			case FTString:
			default:
				/* (read unsupported field types as string values) */
				if (Tcl_ListObjAppendElement(interp, attributes,
						Tcl_NewStringObj(DBFReadStringAttribute(shapefile->dbf, recordId, fieldi), -1)) != TCL_OK) {
					return TCL_ERROR;
				}
				break;
		}
	}
	
	/* return attribute list */
	Tcl_SetObjResult(interp, attributes);
	return TCL_OK;
}

/* attributes - get dbf attribute values of specified feature */
int shapefile_cmd_attributes(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int recordId;
	
	
	
	
	if (objc < 2 || objc > 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "?index ?attributes??");
		return TCL_ERROR;
	}
	
	/* if reading or writing a specific record, get the record index */
	if (objc == 3 || objc == 4) {
		if (Tcl_GetIntFromObj(interp, objv[2], &recordId) != TCL_OK) {
			return TCL_ERROR;
		}
	}
	
	if (objc == 4) {
		/* output; write provided attributes to specified record index */
		if (recordId == -1) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid record index %d (use write command)", recordId));
			return TCL_ERROR;
		}

		/* if successful, sets interp's result to the recordId of the written record */
		if (shapefile_util_attrWrite(interp, shapefile, recordId, 1 /* validate */, objv[3]) != TCL_OK) {
			return TCL_ERROR;
		}
	}
	else if (objc == 3) {
		/* input; return attributes read from specified record index */
		/* if successful, sets interp result to attribute record value list */
		if (shapefile_util_attrRead(interp, shapefile, recordId) != TCL_OK) {
			return TCL_ERROR;
		}
	} 
	else {
		/* slurp input; return list of all records (each an attribute list) */
		Tcl_Obj *recordList;
		int dbfCount;
		
		recordList = Tcl_NewListObj(0, NULL);
		dbfCount = DBFGetRecordCount(shapefile->dbf);

		for (recordId = 0; recordId < dbfCount; recordId++) {
			
			if (shapefile_util_attrRead(interp, shapefile, recordId) != TCL_OK) {
				return TCL_ERROR;
			}
						
			/* append this record's attribute list to the record list */
			if (Tcl_ListObjAppendElement(interp, recordList, Tcl_GetObjResult(interp)) != TCL_OK) {
				return TCL_ERROR;
			}
			
			Tcl_ResetResult(interp);
		}
		
		Tcl_SetObjResult(interp, recordList);
	}
		
	return TCL_OK;
}

/* write - create a new record */
int shapefile_cmd_write(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int outputFeatureId, outputAttributeId;
	
	if (objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "coords attributes");
		return TCL_ERROR;
	}
	
	if (shapefile->readonly) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("cannot write to readonly shapefile"));
		return TCL_ERROR;
	}
	
	/* pre-validate attributes before writing anything */
	if (shapefile_util_attrValidate(interp, shapefile, objv[3]) != TCL_OK) {
		return TCL_ERROR;
	}
	Tcl_ResetResult(interp);
	
	/* write the new feature coords (nothing written if coordWrite fails) */
	if (shapefile_util_coordWrite(interp, shapefile, -1, objv[2]) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp), &outputFeatureId) != TCL_OK) {
		return TCL_ERROR;
	}
	Tcl_ResetResult(interp);
	
	/* write the pre-validated attribute record */
	if (shapefile_util_attrWrite(interp, shapefile, -1, 0, objv[3]) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp), &outputAttributeId) != TCL_OK) {
		return TCL_ERROR;
	}

	/* assert that the new feature and attribute record ids match */
	if (outputFeatureId != outputAttributeId) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("new coord index (%d) does not match new attribute record index (%d)", outputFeatureId, outputAttributeId));
		return TCL_ERROR;
	}
	
	/* result is new entity id, as set by attrWrite */
	return TCL_OK;
}

/* dispatches subcommands */
int shapefile_commands(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	int subcommandIndex;
	static const char *subcommandNames[] = {
			"attributes", "bounds", "close", "coords", "count", "fields", "mode", "type", "write", NULL
	};
	Tcl_ObjCmdProc *subcommands[] = {
			shapefile_cmd_attributes, shapefile_cmd_bounds, shapefile_cmd_close,
			shapefile_cmd_coords, shapefile_cmd_count, shapefile_cmd_fields,
			shapefile_cmd_mode, shapefile_cmd_type, shapefile_cmd_write
	};
	
	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg ...?");
		return TCL_ERROR;
	}
	
	/* identify subcommand, or set result to error message w/valid cmd list */
	/* specify 0 instead of TCL_EXACT to match unambiguous partial cmd names */
	if (Tcl_GetIndexFromObj(interp, objv[1], subcommandNames, "subcommand",
			TCL_EXACT, &subcommandIndex) != TCL_OK) {
		return TCL_ERROR;
	}
		
	/* invoke the requested subcommand directly, passing on all arguments */
	return subcommands[subcommandIndex](clientData, interp, objc, objv);
}

/*
	This command opens a new or existing shapefile.
	This command creates and returns a uniquely named new ensemble command
	associated with the opened shapefile (handled by shapefile_commands).
*/	
int shapetcl_cmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile;
	const char *path;
	char cmdName[16];
	int readonly = 0; /* readwrite access by default */
	SHPHandle shp;
	DBFHandle dbf;
	int shpType;
	
	if (objc < 2 || objc > 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "path ?mode?|?type fields?");
		return TCL_ERROR;
	}

	path = Tcl_GetString(objv[1]);

	if (objc == 3) {
		/* opening an existing file, and an access mode is explicitly set */
		const char *mode = Tcl_GetString(objv[2]);
		if (strcmp(mode, "readonly") == 0) {
			readonly = 1;
		}
		else if (strcmp(mode, "readwrite") == 0) {
			readonly = 0;
		}
		else {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid mode \"%s\": should be readonly or readwrite", mode));
			return TCL_ERROR;
		}
	}
	
	if (objc == 4) {
		/* create a new file; access is readwrite. */
		
		const char *shpTypeName = Tcl_GetString(objv[2]);
		int fieldSpecCount;
		Tcl_Obj **fieldSpec;
		int fieldi;
		const char *type, *name;
		int width, precision;
		
		if (strcmp(shpTypeName, "point") == 0) {
			shpType = SHPT_POINT;
		} else if (strcmp(shpTypeName, "arc") == 0) {
			shpType = SHPT_ARC;
		} else if (strcmp(shpTypeName, "polygon") == 0) {
			shpType = SHPT_POLYGON;
		} else if (strcmp(shpTypeName, "multipoint") == 0) {
			shpType = SHPT_MULTIPOINT;
		} else if (strcmp(shpTypeName, "pointm") == 0) {
			shpType = SHPT_POINTM;
		} else if (strcmp(shpTypeName, "arcm") == 0) {
			shpType = SHPT_ARCM;
		} else if (strcmp(shpTypeName, "polygonm") == 0) {
			shpType = SHPT_POLYGONM;
		} else if (strcmp(shpTypeName, "multipointm") == 0) {
			shpType = SHPT_MULTIPOINTM;
		} else if (strcmp(shpTypeName, "pointz") == 0) {
			shpType = SHPT_POINTZ;
		} else if (strcmp(shpTypeName, "arcz") == 0) {
			shpType = SHPT_ARCZ;
		} else if (strcmp(shpTypeName, "polygonz") == 0) {
			shpType = SHPT_POLYGONZ;
		} else if (strcmp(shpTypeName, "multipointz") == 0) {
			shpType = SHPT_MULTIPOINTZ;
		} else {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid shape type \"%s\"", shpTypeName));
			return TCL_ERROR;
		}
				
		if (Tcl_ListObjGetElements(interp, objv[3], &fieldSpecCount, &fieldSpec) != TCL_OK) {
			return TCL_ERROR;
		}
		
		if (fieldSpecCount % 4 != 0) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("each field requires four values (type, name, width, and precision)"));
			return TCL_ERROR;
		}
		
		if (fieldSpecCount == 0) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("at least one field is required"));
			return TCL_ERROR;
		}
		
		/* validate field specifications before creating dbf */	
		for (fieldi = 0; fieldi < fieldSpecCount; fieldi += 4) {
			
			type = Tcl_GetString(fieldSpec[fieldi]);
			if (strcmp(type, "string") != 0 && strcmp(type, "integer") != 0 && strcmp(type, "double") != 0) {
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid field type \"%s\": should be string, integer, or double", type));
				return TCL_ERROR;
			}
			
			name = Tcl_GetString(fieldSpec[fieldi + 1]);
			if (strlen(name) > 10) {
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("field name \"%s\" too long: 10 characters maximum"));
				return TCL_ERROR;
			}
			
			if (Tcl_GetIntFromObj(interp, fieldSpec[fieldi + 2], &width) != TCL_OK) {
				return TCL_ERROR;
			}
			if (strcmp(type, "integer") == 0 && width > 10) {
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("integer width >10 (%d) would become double", width));
				return TCL_ERROR;
			}
			
			if (Tcl_GetIntFromObj(interp, fieldSpec[fieldi + 3], &precision) != TCL_OK) {
				return TCL_ERROR;
			}
			if (strcmp(type, "double") == 0 && width <= 10 && precision == 0) {
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("double width <=10 (%d) with 0 precision would become integer", width));
				return TCL_ERROR;
			}
		}
		
		if ((dbf = DBFCreate(path)) == NULL) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to create attribute table for \"%s\"", path));
			return TCL_ERROR;
		}
		
		/* add fields to the dbf */
		for (fieldi = 0; fieldi <  fieldSpecCount; fieldi += 4) {
			type = Tcl_GetString(fieldSpec[fieldi]);
			name = Tcl_GetString(fieldSpec[fieldi + 1]);
			Tcl_GetIntFromObj(interp, fieldSpec[fieldi + 2], &width);
			Tcl_GetIntFromObj(interp, fieldSpec[fieldi + 3], &precision);
			if (strcmp(type, "integer") == 0) {
				if (DBFAddField(dbf, name, FTInteger, width, 0) == -1) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to create integer attribute field \"%s\"", name));
					DBFClose(dbf);
					return TCL_ERROR;
				}
			}
			else if (strcmp(type, "double") == 0) {
				if (DBFAddField(dbf, name, FTDouble, width, precision) == -1) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to create double attribute field \"%s\"", name));
					DBFClose(dbf);
					return TCL_ERROR;
				}
			}
			else if (strcmp(type, "string") == 0) {
				if (DBFAddField(dbf, name, FTString, width, 0) == -1) {
					Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to create string attribute field \"%s\"", name));
					DBFClose(dbf);
					return TCL_ERROR;
				}
			}
		}
		
		if ((shp = SHPCreate(path, shpType)) == NULL) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to create shapefile for \"%s\"", path));
			DBFClose(dbf);
			return TCL_ERROR;
		}
	}
	else {		
		
		/* open an existing shapefile */
		int shpCount, dbfCount;
		
		if ((dbf = DBFOpen(path, readonly ? "rb" : "rb+")) == NULL) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to open attribute table for \"%s\"", path));
			return TCL_ERROR;
		}
		
		if (DBFGetFieldCount(dbf) == 0) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("attribute table for \"%s\" contains no fields", path));
			DBFClose(dbf);
			return TCL_ERROR;
		}
		
		if ((shp = SHPOpen(path, readonly ? "rb" : "rb+")) == NULL) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to open shapefile for \"%s\"", path));
			DBFClose(dbf);
			return TCL_ERROR;
		}
		
		/* Only types we don't handle are SHPT_NULL and SHPT_MULTIPATCH */
		SHPGetInfo(shp, &shpCount, &shpType, NULL, NULL);
		if (shpType != SHPT_POINT && shpType != SHPT_POINTM && shpType != SHPT_POINTZ &&
				shpType != SHPT_ARC && shpType != SHPT_ARCM && shpType != SHPT_ARCZ &&
				shpType != SHPT_POLYGON && shpType != SHPT_POLYGONM && shpType != SHPT_POLYGONZ &&
				shpType != SHPT_MULTIPOINT && shpType != SHPT_MULTIPOINTM && shpType != SHPT_MULTIPOINTZ) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("unsupported shape type (%d)", shpType));
			DBFClose(dbf);
			SHPClose(shp);
			return TCL_ERROR;
		}
		
		/* Valid shapefiles must have matching number of features and attribute records */
		dbfCount = DBFGetRecordCount(dbf);
		if (dbfCount != shpCount) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("shapefile feature count (%d) does not match attribute record count (%d)", shpCount, dbfCount));
			DBFClose(dbf);
			SHPClose(shp);
			return TCL_ERROR;
		}
	}
	
	if ((shapefile = (ShapefilePtr)ckalloc(sizeof(shapetcl_shapefile))) == NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to allocate shapefile command data"));;
		DBFClose(dbf);
		SHPClose(shp);
		return TCL_ERROR;
	}
	shapefile->shp = shp;
	shapefile->dbf = dbf;	
	shapefile->readonly = readonly;
	
	sprintf(cmdName, "shapefile.%04X", COMMAND_COUNT++);
	if (Tcl_CreateObjCommand(interp, cmdName, shapefile_commands, (ClientData)shapefile, shapefile_util_delete) == NULL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to create command for %s", cmdName));
		DBFClose(dbf);
		SHPClose(shp);
		ckfree((char *)shapefile);
		return TCL_ERROR;
	}
	Tcl_CreateExitHandler(shapefile_util_close, (ClientData)shapefile);
	Tcl_SetObjResult(interp, Tcl_NewStringObj(cmdName, -1));
	
	return TCL_OK;
}

int Shapetcl_Init(Tcl_Interp *interp) {
	
	if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
		return TCL_ERROR;
	}
	
	if (Tcl_PkgProvide(interp, "Shapetcl", "0.1") != TCL_OK) {
		return TCL_ERROR;
	}
	
	Tcl_CreateObjCommand(interp, "shapefile", shapetcl_cmd, NULL, NULL);
	
	return TCL_OK;
}
