#include <stdio.h>
#include <string.h>
#include <tcl.h>
#include "shapefil.h"

#include "shapetcl.h"
#include "shapetcl_info.h"
#include "shapetcl_coords.h"
#include "shapetcl_attributes.h"

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
