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

int shapefile_cmd_close(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	const char *cmdName = Tcl_GetStringFromObj(objv[0], NULL);

	if (objc > 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	}
	
	printf("closing shp\n");
	SHPClose(shapefile->shp);
	shapefile->shp = NULL;
	
	printf("closing dbf\n");
	DBFClose(shapefile->dbf);
	shapefile->dbf = NULL;
	
	printf("deleting command %s\n", cmdName);
	/* triggers the deleteProc shapetcl_cleanup */
	Tcl_DeleteCommand(interp, cmdName);
	
	return TCL_OK;
}

/* turn this into a general info getter; allow shapefile path lookup, etc. */
int shapefile_cmd_info(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	const char *query;

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "setting");
		return TCL_ERROR;
	}
	
	query = Tcl_GetStringFromObj(objv[2], NULL);
	
	if (strcmp(query, "readonly") == 0) {
		Tcl_SetObjResult(interp, Tcl_NewBooleanObj(shapefile->readonly));
	}
	else {
		Tcl_SetResult(interp, "unrecognized setting", TCL_STATIC);
		return TCL_ERROR;
	}
	
	return TCL_OK;
}

int shapefile_cmd_count(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int shpCount, dbfCount;
	
	if (objc > 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	}
	
	SHPGetInfo(shapefile->shp, &shpCount, NULL, NULL, NULL);
	
	dbfCount = DBFGetRecordCount(shapefile->dbf);
	
	if (shpCount != dbfCount) {
		Tcl_SetResult(interp, "shp count does not match dbf count", TCL_STATIC);
		return TCL_ERROR;
	}
	
	Tcl_SetObjResult(interp, Tcl_NewIntObj(shpCount));
	return TCL_OK;
}

int shapefile_cmd_type(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int shpType;

	if (objc > 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	}
	
	SHPGetInfo(shapefile->shp, NULL, &shpType, NULL, NULL);
	
	switch (shpType) {
		case SHPT_POINT:
			Tcl_SetResult(interp, "point", TCL_STATIC);
			break;
		case SHPT_ARC:
			Tcl_SetResult(interp, "arc", TCL_STATIC);
			break;
		case SHPT_POLYGON:
			Tcl_SetResult(interp, "polygon", TCL_STATIC);
			break;
		case SHPT_MULTIPOINT:
			Tcl_SetResult(interp, "multipoint", TCL_STATIC);
			break;
		default:
			/* unsupported type */
			/* should try to notice unsupported types on open */
			Tcl_SetResult(interp, "unsupported type", TCL_STATIC);
			return TCL_ERROR;
			break;
	}
	
	return TCL_OK;
}

int shapefile_cmd_bounds(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int shpCount;
	double min[4], max[4];
	Tcl_Obj *bounds;
		
	if (objc > 3) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	}
	
	/* get the file count & bounds now; we'll need the count to validate the
	   feature index, if given, in which case we'll replace min & max result. */
	SHPGetInfo(shapefile->shp, &shpCount, NULL, min, max);
	
	if (objc == 3) {
		int featureId;
		SHPObject *obj;
		
		if (Tcl_GetIntFromObj(interp, objv[2], &featureId) != TCL_OK) {
			return TCL_ERROR;
		}
		
		if (featureId < 0 || featureId >= shpCount) {
			Tcl_SetResult(interp, "index out of bounds", TCL_STATIC);
			return TCL_ERROR;
		}
		
		if ((obj = SHPReadObject(shapefile->shp, featureId)) == NULL) {
			Tcl_SetResult(interp, "cannot read shape", TCL_STATIC);
			return TCL_ERROR;
		}
		
		if (obj->nSHPType == SHPT_NULL) {
			Tcl_SetResult(interp, "no bounds for null feature", TCL_STATIC);
			return TCL_ERROR;
		}
		
		min[0] = obj->dfXMin; min[1] = obj->dfYMin; min[2] = obj->dfZMin; min[3] = obj->dfMMin;
		max[0] = obj->dfXMax; max[1] = obj->dfYMax; max[2] = obj->dfZMax; max[3] = obj->dfMMax;
		
		SHPDestroyObject(obj);
	}
	
	/* Presently just returning the 2d x,y bounds. Consider returning bounds in
	   an appropriate number of dimensions based on shape type. */
	bounds = Tcl_NewListObj(0, NULL);
	Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(min[0]));
	Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(min[1]));
	Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(max[0]));
	Tcl_ListObjAppendElement(interp, bounds, Tcl_NewDoubleObj(max[1]));
	Tcl_SetObjResult(interp, bounds);

	return TCL_OK;
}

/* shapefile_cmd_read */

/* shapefile_cmd_write */

/* shapefile_cmd_fields */
/* note that the dbf api does support adding/removing fields to extant files */

/* dispatches subcommands */
int shapefile_commands(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	const char *subcommand;
	
	if (objc < 2) {
		Tcl_SetResult(interp, "missing subcommand", TCL_STATIC);
		return TCL_ERROR;
	}
	
	subcommand = Tcl_GetStringFromObj(objv[1], NULL);
	
	if (strcmp(subcommand, "close") == 0)
		return shapefile_cmd_close(clientData, interp, objc, objv);
	else if (strcmp(subcommand, "count") == 0)
		return shapefile_cmd_count(clientData, interp, objc, objv);
	else if (strcmp(subcommand, "type") == 0)
		return shapefile_cmd_type(clientData, interp, objc, objv);
	else if (strcmp(subcommand, "bounds") == 0)
		return shapefile_cmd_bounds(clientData, interp, objc, objv);
	else if (strcmp(subcommand, "info") == 0)
		return shapefile_cmd_info(clientData, interp, objc, objv);
	
	Tcl_SetResult(interp, "unrecognized subcommand", TCL_STATIC);
	return TCL_ERROR;
}

void shapetcl_cleanup(ClientData clientData) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	printf("ckfreeing ShapefilePtr\n");
	ckfree((char *)shapefile);
}

/*
	The shapetcl command opens a new or existing shapefile.
	This command creates and returns a uniquely named new ensemble command
	associated with the opened shapefile (handled by shapefile_commands).
*/	
int shapetcl_command(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile;
	const char *path;
	char cmdName[16];
	int readonly = 0; /* readwrite access by default */
	
	if (objc < 2 || objc > 4) {
		Tcl_SetResult(interp, "shapetcl path [rb|rb+]|type fields", TCL_STATIC);
		return TCL_ERROR;
	}

	path = Tcl_GetStringFromObj(objv[1], NULL);

	if (objc == 3) {
		/* opening an existing file, and an access mode is explicitly set */
		const char *mode = Tcl_GetStringFromObj(objv[2], NULL);
		if (strcmp(mode, "rb") == 0) {
			readonly = 1;
		}
		else if (strcmp(mode, "rb+") == 0) {
			readonly = 0;
		}
		else {
			Tcl_SetResult(interp, "mode should be rb or rb+", TCL_STATIC);
			return TCL_ERROR;
		}
	}
	
	/* need to ckfree this manually on any subsequent TCL_ERROR here? */
	shapefile = (ShapefilePtr)ckalloc(sizeof(shapetcl_shapefile));
	shapefile->readonly = readonly;
	
	if (objc == 4) {
		/* create a new file; access is readwrite. */
		
		const char *shpTypeName = Tcl_GetStringFromObj(objv[2], NULL);
		int shpType;
		
		if (strcmp(shpTypeName, "point") == 0)
			shpType = SHPT_POINT;
		else if (strcmp(shpTypeName, "arc") == 0)
			shpType = SHPT_ARC;
		else if (strcmp(shpTypeName, "polygon") == 0)
			shpType = SHPT_POLYGON;
		else if (strcmp(shpTypeName, "multipoint") == 0)
			shpType = SHPT_MULTIPOINT;
		else {
			Tcl_SetResult(interp, "unrecognized shape type", TCL_STATIC);
			return TCL_ERROR;
		}
		
		if ((shapefile->shp = SHPCreate(path, shpType)) == NULL) {
			Tcl_SetResult(interp, "cannot create .shp", TCL_STATIC);
			return TCL_ERROR;
		}
		
		if ((shapefile->dbf = DBFCreate(path)) == NULL) {
			Tcl_SetResult(interp, "cannot create .dbf", TCL_STATIC);
			return TCL_ERROR;
		}
		
		/* add fields to dbf now based on field specs in objv[3] */
		
	}
	else {		
		if ((shapefile->shp = SHPOpen(path, shapefile->readonly ? "rb" : "rb+")) == NULL) {
			Tcl_SetResult(interp, "cannot open .shp", TCL_STATIC);
			return TCL_ERROR;
		}
		
		if ((shapefile->dbf = DBFOpen(path, shapefile->readonly ? "rb" : "rb+")) == NULL) {
			Tcl_SetResult(interp, "cannot open .dbf", TCL_STATIC);
			return TCL_ERROR;
		}
	}
	
	sprintf(cmdName, "shapefile.%04X", COMMAND_COUNT++);
	Tcl_CreateObjCommand(interp, cmdName, shapefile_commands, (ClientData)shapefile, shapetcl_cleanup);
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
	
	Tcl_CreateObjCommand(interp, "shapetcl", shapetcl_command, NULL, NULL);
	
	return TCL_OK;
}
