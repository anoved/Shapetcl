#include <stdio.h>
#include <string.h>
#include <tcl.h>
#include "shapefil.h"

#include "shapetcl.h"

static int COMMAND_COUNT = 0;

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

/* coords - get or set feature coordinates */
int shapefile_cmd_coords(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int featureId;
	int allCoords, xyOnly;
	
	allCoords = util_flagIsPresent(objc, objv, "-all");
	xyOnly = util_flagIsPresent(objc, objv, "-xy");
	if (xyOnly) {
		objc--;
	}
	if (allCoords) {
		xyOnly = 0;
		objc--;
	}
	
	if (objc < 2 || objc > 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "?index ?coords?? ?-all|-xy?");
		return TCL_ERROR;
	}
	
	if (objc == 3 || objc == 4) {
		if (Tcl_GetIntFromObj(interp, objv[2], &featureId) != TCL_OK) {
			return TCL_ERROR;
		}
	}
		
	if (objc == 4) {
		/* output mode */
		if (featureId == -1) {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("invalid feature index %d (use write command)", featureId));
			return TCL_ERROR;
		}

		/* if shape output is successful, interp result is set to output feature id */
		if (shapefile_util_coordWrite(interp, shapefile, featureId, objv[3]) != TCL_OK) {
			return TCL_ERROR;
		}
	}
	else if (objc == 3) {
		/* input mode - read and return coordinates from featureId */
		/* if shape input is successful, interp result is set to coordinate list */
		if (shapefile_util_coordRead(interp, shapefile, featureId, allCoords, xyOnly) != TCL_OK) {
			return TCL_ERROR;
		}
	}
	else {
		/* slurp input */
		Tcl_Obj *featureList;
		int shpCount;
		
		featureList = Tcl_NewListObj(0, NULL);
		SHPGetInfo(shapefile->shp, &shpCount, NULL, NULL, NULL);
		
		for (featureId = 0; featureId < shpCount; featureId++) {
			
			if (shapefile_util_coordRead(interp, shapefile, featureId, allCoords, xyOnly) != TCL_OK) {
				return TCL_ERROR;
			}
			
			if (Tcl_ListObjAppendElement(interp, featureList, Tcl_GetObjResult(interp)) != TCL_OK) {
				return TCL_ERROR;
			}
			
			Tcl_ResetResult(interp);
		}
		
		Tcl_SetObjResult(interp, featureList);
	}
	
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
			"attributes", "bounds", "close", "coords", "count", "fields", "mode", "type", "write"
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
