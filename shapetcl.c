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

typedef struct {
	
	/* field characteristics */
	DBFFieldType type;
	int width;
	int precision;
	
	/* specified value */
	int intValue;
	double doubleValue;
	const char *stringValue;
	int nullValue;
	
} attributeValue;
typedef attributeValue *AttributeValuePtr;

static int COMMAND_COUNT = 0;

/* shapefile closer - invoked if manually closed or automatically on exit */
void shapefile_util_close(ClientData clientData) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	/*printf("shapefile_util_close %lX\n", (unsigned long int)shapefile);*/
	SHPClose(shapefile->shp);
	shapefile->shp = NULL;
	DBFClose(shapefile->dbf);
	shapefile->dbf = NULL;
}

/* exit handler - invoked if shapefile is not manually closed prior to exit */
void shapefile_util_exit(ClientData clientData) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	/*printf("shapefile_util_exit %lX\n", (long unsigned int)shapefile);*/
	shapefile_util_close(shapefile);
}

/* delete proc - invoked if shapefile is manually closed. deletes exit handler */
void shapefile_util_delete(ClientData clientData) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	/*printf("shapefile_util_delete %lX\n", (long unsigned int)shapefile);*/
	Tcl_DeleteExitHandler(shapefile_util_exit, shapefile);
	ckfree((char *)shapefile);
}

/* close - flush shapefile and delete associated command */
int shapefile_cmd_close(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;

	if (objc > 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	}
	
	shapefile_util_close(shapefile);
	
	Tcl_DeleteCommand(interp, Tcl_GetStringFromObj(objv[0], NULL));
	
	return TCL_OK;
}

/* mode - report shapefile access mode (rb readonly, rb+ readwrite) */
int shapefile_cmd_mode(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	}
	
	if (shapefile->readonly)
		Tcl_SetResult(interp, "rb", TCL_STATIC);
	else
		Tcl_SetResult(interp, "rb+", TCL_STATIC);
	
	return TCL_OK;
}

/* count - report number of entities in shapefile (shp & dbf should match) */
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

/* type - report type of geometry in shapefile */
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

/* bounds - report 2d bounds of shapefile or specified feature */
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

/* fields - report attribute table field definitions */
int shapefile_cmd_fields(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int fieldCount, fieldi;
	Tcl_Obj *fieldSpec;
	char name[12];
	int width, precision;
	DBFFieldType fieldType;

	if (objc > 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	}
	
	fieldCount = DBFGetFieldCount(shapefile->dbf);
	fieldSpec = Tcl_NewListObj(0, NULL);
	
	for (fieldi = 0; fieldi < fieldCount; fieldi++) {
		fieldType = DBFGetFieldInfo(shapefile->dbf, fieldi, name, &width, &precision);		
		switch (fieldType) {
			case FTString:
				if (Tcl_ListObjAppendElement(interp, fieldSpec, Tcl_NewStringObj("string", -1)) != TCL_OK)
					return TCL_ERROR;
				break;
			case FTInteger:
				if (Tcl_ListObjAppendElement(interp, fieldSpec, Tcl_NewStringObj("integer", -1)) != TCL_OK)
					return TCL_ERROR;
				break;
			case FTDouble:
				if (Tcl_ListObjAppendElement(interp, fieldSpec, Tcl_NewStringObj("double", -1)) != TCL_OK)
					return TCL_ERROR;
				break;
			default:
				/* represent unsupported field types by numeric type ID instead of descriptive name */
				if (Tcl_ListObjAppendElement(interp, fieldSpec, Tcl_NewIntObj((int)fieldType)) != TCL_OK)
					return TCL_ERROR;
				break;
		}
		
		if (Tcl_ListObjAppendElement(interp, fieldSpec, Tcl_NewStringObj(name, -1)) != TCL_OK)
			return TCL_ERROR;
		if (Tcl_ListObjAppendElement(interp, fieldSpec, Tcl_NewIntObj(width)) != TCL_OK)
			return TCL_ERROR;
		if (Tcl_ListObjAppendElement(interp, fieldSpec, Tcl_NewIntObj(precision)) != TCL_OK)
			return TCL_ERROR;
	}
	
	Tcl_SetObjResult(interp, fieldSpec);
	return TCL_OK;
}

int shapefile_util_coordWrite(Tcl_Interp *interp, ShapefilePtr shapefile, int featureId, Tcl_Obj *coordParts) {
	int shapeType, featureCount;
	int outputFeatureId;
	int *partStarts;
	Tcl_Obj *coords, *coord;
	int part, partCount, partCoord, partCoordCount;
	int vertex, vertexCount, partVertexCount;
	double *xCoords, *yCoords, x, y;
	SHPObject *shape;
	
	if (shapefile->readonly) {
		Tcl_SetResult(interp, "cannot write coordinates with readonly access", TCL_STATIC);
		return TCL_ERROR;
	}
	
	SHPGetInfo(shapefile->shp, &featureCount, &shapeType, NULL, NULL);
	if (featureId < -1 || featureId >= featureCount) {
		Tcl_SetResult(interp, "invalid output feature id", TCL_STATIC);
		return TCL_ERROR;
	}
	/* a featureId of -1 indicates a new feature should be output */
	
	if (Tcl_ListObjLength(interp, coordParts, &partCount) != TCL_OK)
		return TCL_ERROR;
	
	partStarts = (int *)ckalloc(sizeof(int) * partCount);
	xCoords = NULL; yCoords = NULL;

	vertex = 0;
	vertexCount = 0;
	
	for (part = 0; part < partCount; part++) {
		partStarts[part] = vertex;
		
		/* get the coordinates that comprise this part */
		if (Tcl_ListObjIndex(interp, coordParts, part, &coords) != TCL_OK)
			return TCL_ERROR;
		
		/* verify that the coordinate list has a valid number of elements */
		if (Tcl_ListObjLength(interp, coords, &partCoordCount) != TCL_OK)
			return TCL_ERROR;
		if (partCoordCount % 2 != 0) {
			Tcl_SetResult(interp, "coordinate list malformed", TCL_STATIC);
			return TCL_ERROR;
		}
		partVertexCount = partCoordCount / 2;
		
		/* do we handle empty parts? */
		/* some shape types have minimum number of vertices */
		/* some shapes have maximum number of parts (eg SHPT_POINT) */
		
		/* add space for this part's vertices */
		vertexCount += partVertexCount;
		xCoords = (double *)ckrealloc((char *)xCoords, sizeof(double) * vertexCount);
		yCoords = (double *)ckrealloc((char *)yCoords, sizeof(double) * vertexCount);
		
		for (partCoord = 0; partCoord < partCoordCount; partCoord += 2) {
			
			if (Tcl_ListObjIndex(interp, coords, partCoord, &coord) != TCL_OK)
				return TCL_ERROR;
			if (Tcl_GetDoubleFromObj(interp, coord, &x) != TCL_OK)
				return TCL_ERROR;
			xCoords[vertex] = x;
			
			if (Tcl_ListObjIndex(interp, coords, partCoord + 1, &coord) != TCL_OK)
				return TCL_ERROR;
			if (Tcl_GetDoubleFromObj(interp, coord, &y) != TCL_OK)
				return TCL_ERROR;
			yCoords[vertex] = y;
			
			vertex++;
		}
	}
	
	/* assemble the coordinate lists into a new shape */
	if ((shape = SHPCreateObject(shapeType, featureId, partCount,
			partStarts, NULL, vertexCount, xCoords, yCoords, NULL, NULL)) == NULL) {
		Tcl_SetResult(interp, "cannot create shape", TCL_STATIC);
		return TCL_ERROR;
	}
	
	/* correct the shape's vertex order, if necessary */
	SHPRewindObject(shapefile->shp, shape);
	
	/* write the shape to the shapefile */
	if ((outputFeatureId = SHPWriteObject(shapefile->shp, featureId, shape)) == -1) {
		Tcl_SetResult(interp, "cannot write shape", TCL_STATIC);
		return TCL_ERROR;
	}
	
	SHPDestroyObject(shape);
	ckfree((char *)partStarts);
	ckfree((char *)xCoords);
	ckfree((char *)yCoords);
	
	Tcl_SetObjResult(interp, Tcl_NewIntObj(outputFeatureId));
	return TCL_OK;
}

int shapefile_util_coordRead(Tcl_Interp *interp, ShapefilePtr shapefile, int featureId) {
	SHPObject *shape;
	Tcl_Obj *coordParts;
	int featureCount, part, partCount, vertex, vertexStart, vertexStop;
	
	SHPGetInfo(shapefile->shp, &featureCount, NULL, NULL, NULL);
	if (featureId < 0 || featureId >= featureCount) {
		Tcl_SetResult(interp, "invalid feature id", TCL_STATIC);
		return TCL_ERROR;
	}
	
	if ((shape = SHPReadObject(shapefile->shp, featureId)) == NULL) {
		Tcl_SetResult(interp, "cannot read feature", TCL_STATIC);
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
			if (Tcl_ListObjAppendElement(interp, coords, Tcl_NewDoubleObj(shape->padfX[vertex])) != TCL_OK)
				return TCL_ERROR;
			if (Tcl_ListObjAppendElement(interp, coords, Tcl_NewDoubleObj(shape->padfY[vertex])) != TCL_OK)
				return TCL_ERROR;
		}
		
		/* add this part's coordinate list to the feature's part list */
		if (Tcl_ListObjAppendElement(interp, coordParts, coords) != TCL_OK)
			return TCL_ERROR;
		
		/* advance vertex indices to the next part (disregarded if none) */
		vertexStart = vertex;
		if (part + 2 < partCount)
			vertexStop = shape->panPartStart[part + 1];
		else
			vertexStop = shape->nVertices;
		part++;
	}
	
	SHPDestroyObject(shape);
	
	Tcl_SetObjResult(interp, coordParts);
	return TCL_OK;
}

/* coords - get 2d coordinates of specified feature */
int shapefile_cmd_coords(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	ShapefilePtr shapefile = (ShapefilePtr)clientData;
	int featureId;
	
	if (objc != 3 && objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "ID [coordinates]");
		return TCL_ERROR;
	}
	
	if (Tcl_GetIntFromObj(interp, objv[2], &featureId) != TCL_OK)
		return TCL_ERROR;
	
	/* validation of featureId is performed by coord input/output blocks */
	
	if (objc == 4) {
		/* output mode */
		/* if shape output is successful, interp result is set to output feature id */
		if (shapefile_util_coordWrite(interp, shapefile, featureId, objv[3]) != TCL_OK)
			return TCL_ERROR;
	}
	else {
		/* input mode - read and return coordinates from featureId */
		/* if shape input is successful, interp result is set to coordinate list */
		if (shapefile_util_coordRead(interp, shapefile, featureId) != TCL_OK)
			return TCL_ERROR;
	}
	
	return TCL_OK;
}

int shapefile_util_attrWrite(Tcl_Interp *interp, ShapefilePtr shapefile, int recordId, Tcl_Obj *attrList) {
	Tcl_Obj *attr;
	int fieldi, fieldCount, attrCount, dbfCount;
	char numericStringValue[64];
	int checkForTruncation = 1;
	AttributeValuePtr attributes;
	
	if (shapefile->readonly) {
		Tcl_SetResult(interp, "cannot write attributes with readonly access", TCL_STATIC);
		return TCL_ERROR;
	}
	
	dbfCount = DBFGetRecordCount(shapefile->dbf);
	if (recordId < -1 || recordId >= dbfCount) {
		Tcl_SetResult(interp, "invalid record id", TCL_STATIC);
		return TCL_ERROR;
	}
	if (recordId == -1)
		recordId = dbfCount;
	
	/* now recordId is either the id of an existing record to overwrite,
	   or the id-elect of a new record we will create. Unlike shape output,
	   we use predicted id instead of -1 because each field value must be
	   written by a separate DBF*Writer function call, and we don't want
	   to write each value to a different new record! */
	
	/* verify the provided attribute value list matches field count */
	fieldCount = DBFGetFieldCount(shapefile->dbf);
	if (Tcl_ListObjLength(interp, attrList, &attrCount) != TCL_OK)
		return TCL_ERROR;
	if (attrCount != fieldCount) {
		Tcl_SetResult(interp, "attribute count does not match field count", TCL_STATIC);
		return TCL_ERROR;
	}
	
	/* currently prone to memory leaks if any error occurs before the end - not free()ed */
	attributes = (AttributeValuePtr)ckalloc(sizeof(AttributeValuePtr) * fieldCount);
	
	/* validation pass - confirm field values can be parsed as field type
	   (and perhaps check for obvious cases of truncation) before output */
	for (fieldi = 0; fieldi < fieldCount; fieldi++) {
		
		/* grab the attr provided for this field */
		if (Tcl_ListObjIndex(interp, attrList, fieldi, &attr) != TCL_OK) {
			ckfree((char *)attributes);
			return TCL_ERROR;
		}
		
		/* stash the type, width, and precision of this field */
		attributes[fieldi].type = DBFGetFieldInfo(shapefile->dbf, fieldi,
				NULL, &attributes[fieldi].width, &attributes[fieldi].precision);

		/* if it is an empty string {}, write it as a NULL value */
		if (Tcl_GetCharLength(attr) == 0) {
			attributes[fieldi].nullValue = 1;
			continue;
		} else
			attributes[fieldi].nullValue = 0;
				
		switch (attributes[fieldi].type) {
			case FTInteger:
				
				/* can this value be parsed as an integer? */
				if (Tcl_GetIntFromObj(interp, attr, &attributes[fieldi].intValue) != TCL_OK) {
					ckfree((char *)attributes);
					return TCL_ERROR;
				}
				
				/* does this integer fit within the field width? */
				if (checkForTruncation) {
					sprintf(numericStringValue, "%d", attributes[fieldi].intValue);
					if (strlen(numericStringValue) > attributes[fieldi].width) {
						Tcl_SetResult(interp, "integer value would be truncated", TCL_STATIC);
						ckfree((char *)attributes);
						return TCL_ERROR;
					}
				}
				break;
			case FTDouble:
				
				/* can this value be parsed as a double? */
				if (Tcl_GetDoubleFromObj(interp, attr, &attributes[fieldi].doubleValue) != TCL_OK) {
					ckfree((char *)attributes);
					return TCL_ERROR;
				}
				
				/* does this double fit within the field width? */
				if (checkForTruncation) {
					sprintf(numericStringValue, "%.*lf", attributes[fieldi].precision, attributes[fieldi].doubleValue);
					if (strlen(numericStringValue) > attributes[fieldi].width) {
						Tcl_SetResult(interp, "double value would be truncated", TCL_STATIC);
						ckfree((char *)attributes);
						return TCL_ERROR;
					}
				}
				break;
			case FTString:
			
				/* can this value be parsed as a string? */
				if ((attributes[fieldi].stringValue = Tcl_GetString(attr)) == NULL) {
					ckfree((char *)attributes);
					return TCL_ERROR;
				}
				
				/* does this string fit within the field width? */
				if (checkForTruncation && (strlen(attributes[fieldi].stringValue) > attributes[fieldi].width)) {
					Tcl_SetResult(interp, "string value would be truncated", TCL_STATIC);
					ckfree((char *)attributes);
					return TCL_ERROR;
				}
				break;
			default:
				/* write NULL values for all unsupported field types */
				attributes[fieldi].nullValue = 1;
				break;
		}
	}
	
	/* output pass - once the fields are validated, write 'em out. Output is
	   performed separately from validation to avoid mangled/partial output. */
	for (fieldi = 0; fieldi < fieldCount; fieldi++) {
		
		/* null value */
		if (attributes[fieldi].nullValue) {
			if (!DBFWriteNULLAttribute(shapefile->dbf, recordId, fieldi)) {
				Tcl_SetResult(interp, "cannot write NULL attribute. dbf may be invalid", TCL_STATIC);
				ckfree((char *)attributes);
				return TCL_ERROR;
			}
			continue;
		}
		
		/* regular values */
		switch (attributes[fieldi].type) {
			case FTInteger:
				if (!DBFWriteIntegerAttribute(shapefile->dbf, recordId, fieldi, attributes[fieldi].intValue)) {
					Tcl_SetResult(interp, "cannot write integer attribute. dbf may be invalid", TCL_STATIC);
					ckfree((char *)attributes);
					return TCL_ERROR;
				}
				break;
			case FTDouble:
				if (!DBFWriteDoubleAttribute(shapefile->dbf, recordId, fieldi, attributes[fieldi].doubleValue)) {
					Tcl_SetResult(interp, "cannot write double attribute. dbf may be invalid", TCL_STATIC);
					ckfree((char *)attributes);
					return TCL_ERROR;
				}
				break;
			case FTString:
				if (!DBFWriteStringAttribute(shapefile->dbf, recordId, fieldi, attributes[fieldi].stringValue)) {
					Tcl_SetResult(interp, "cannot write string attribute. dbf may be invalid", TCL_STATIC);
					ckfree((char *)attributes);
					return TCL_ERROR;
				}
				break;
			default:
				/* any other field types should have been marked as null during
				   validation pass, and therefore already written as null above */
				break;
		}
	}
	
	ckfree((char *)attributes);
	
	Tcl_SetObjResult(interp, Tcl_NewIntObj(recordId));
	return TCL_OK;
}

int shapefile_util_attrRead(Tcl_Interp *interp, ShapefilePtr shapefile, int recordId) {
	Tcl_Obj *attributes = Tcl_NewListObj(0, NULL);
	int fieldi, fieldCount, dbfCount;
	DBFFieldType fieldType;
	
	fieldCount  = DBFGetFieldCount(shapefile->dbf);
	dbfCount = DBFGetRecordCount(shapefile->dbf);
	if (recordId < 0 || recordId >= dbfCount) {
		Tcl_SetResult(interp, "invalid record id", TCL_STATIC);
		return TCL_ERROR;
	}

	for (fieldi = 0; fieldi < fieldCount; fieldi++) {
		
		/* represent NULL attribute values as {} in return list */
		if (DBFIsAttributeNULL(shapefile->dbf, recordId, fieldi)) {
			if (Tcl_ListObjAppendElement(interp, attributes, Tcl_NewObj()) != TCL_OK)
				return TCL_ERROR;
			continue;
		}
		
		/* interpret attribute value according to field type and append to result list */
		fieldType = DBFGetFieldInfo(shapefile->dbf, fieldi, NULL, NULL, NULL);
		switch (fieldType) {
			case FTInteger:
				if (Tcl_ListObjAppendElement(interp, attributes,
						Tcl_NewIntObj(DBFReadIntegerAttribute(shapefile->dbf, recordId, fieldi))) != TCL_OK)
					return TCL_ERROR;				
				break;
			case FTDouble:
				if (Tcl_ListObjAppendElement(interp, attributes,
						Tcl_NewDoubleObj(DBFReadDoubleAttribute(shapefile->dbf, recordId, fieldi))) != TCL_OK)
					return TCL_ERROR;
				break;
			case FTString:
				if (Tcl_ListObjAppendElement(interp, attributes,
						Tcl_NewStringObj(DBFReadStringAttribute(shapefile->dbf, recordId, fieldi), -1)) != TCL_OK)
					return TCL_ERROR;
				break;
			default:
				/* represent any unsupported field type values as {} (same as NULL) */
				if (Tcl_ListObjAppendElement(interp, attributes, Tcl_NewObj()) != TCL_OK)
					return TCL_ERROR;
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
	
	if (objc != 3 && objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "ID [attributes]");
		return TCL_ERROR;
	}
	
	if (Tcl_GetIntFromObj(interp, objv[2], &recordId) != TCL_OK)
		return TCL_ERROR;
	
	/* validation of recordId is performed within the individual output/input blocks */
	
	if (objc == 4) {
		/* output mode */
		/* if successful, sets interp's result to the recordId of the written record */
		if (shapefile_util_attrWrite(interp, shapefile, recordId, objv[3]) != TCL_OK)
			return TCL_ERROR;
	}
	else {
		/* input mode; return list of attributes from recordId */
		/* if successful, sets interp result to attribute record value list */
		if (shapefile_util_attrRead(interp, shapefile, recordId) != TCL_OK)
			return TCL_ERROR;
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
	
	/* errors in either Write function may leave the shapefile in an invalid
	   state (mismatched number of features and attributes). one strategy to
	   minimize the likelihood of that outcome is to put argument validation
	   in a separate util function that can be called for both first, before
	   proceeding to write. the coord validater may return a SHPObject, and
	   the attributes validator may return a list of values (Tcl_Obj list?) */
	
	/* write the new feature coords */
	if (shapefile_util_coordWrite(interp, shapefile, -1, objv[2]) != TCL_OK)
		return TCL_ERROR;
	if (Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp), &outputFeatureId) != TCL_OK)
		return TCL_ERROR;
	
	Tcl_ResetResult(interp);
	
	/* write the new attribute record */
	if (shapefile_util_attrWrite(interp, shapefile, -1, objv[3]) != TCL_OK)
		return TCL_ERROR;
	if (Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp), &outputAttributeId) != TCL_OK)
		return TCL_ERROR;
	
	/* assert that the new feature and attribute record ids match */
	if (outputFeatureId != outputAttributeId) {
		Tcl_SetResult(interp, "output coord and attribute ids do not match", TCL_STATIC);
		return TCL_ERROR;
	}
	
	/* return the id of the new entity */
	Tcl_SetObjResult(interp, Tcl_NewIntObj(outputFeatureId));
	return TCL_OK;
}

/* dispatches subcommands - to be replaced with namespace ensemble mechanism? */
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
	else if (strcmp(subcommand, "mode") == 0)
		return shapefile_cmd_mode(clientData, interp, objc, objv);
	else if (strcmp(subcommand, "fields") == 0)
		return shapefile_cmd_fields(clientData, interp, objc, objv);
	else if (strcmp(subcommand, "attributes") == 0)
		return shapefile_cmd_attributes(clientData, interp, objc, objv);
	else if (strcmp(subcommand, "coords") == 0)
		return shapefile_cmd_coords(clientData, interp, objc, objv);
	else if (strcmp(subcommand, "write") == 0)
		return shapefile_cmd_write(clientData, interp, objc, objv);	
	
	Tcl_SetResult(interp, "unrecognized subcommand", TCL_STATIC);
	return TCL_ERROR;
}

/*
	The shapetcl command opens a new or existing shapefile.
	This command creates and returns a uniquely named new ensemble command
	associated with the opened shapefile (handled by shapefile_commands).
*/	
int shapetcl_cmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
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
		int fieldSpecCount;
		Tcl_Obj **fieldSpec;
		int fieldi;
		
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
		
		if (Tcl_ListObjGetElements(interp, objv[3], &fieldSpecCount, &fieldSpec) != TCL_OK) {
			return TCL_ERROR;
		}
		
		if (fieldSpecCount % 4 != 0) {
			Tcl_SetResult(interp, "malformed field specification", TCL_STATIC);
			return TCL_ERROR;
		}
		
		for (fieldi = 0; fieldi < fieldSpecCount; fieldi += 4) {
			/*
				fieldi		type
				fieldi + 1	name
				fieldi + 2	width
				fieldi + 3	precision
			*/
			
			const char *type, *name;
			int width, precision;
			
			type = Tcl_GetStringFromObj(fieldSpec[fieldi], NULL);
			name = Tcl_GetStringFromObj(fieldSpec[fieldi + 1], NULL);
			
			if (Tcl_GetIntFromObj(interp, fieldSpec[fieldi + 2], &width) != TCL_OK)
				return TCL_ERROR;

			if (Tcl_GetIntFromObj(interp, fieldSpec[fieldi + 3], &precision) != TCL_OK)
				return TCL_ERROR;
			
			if (strcmp(type, "string") == 0) {
				if (DBFAddField(shapefile->dbf, name, FTString, width, 0) == -1) {
					Tcl_SetResult(interp, "cannot create string field", TCL_STATIC);
					return TCL_ERROR;
				}
			}
			else if (strcmp(type, "integer") == 0) {
				if (DBFAddField(shapefile->dbf, name, FTInteger, width, 0) == -1) {
					Tcl_SetResult(interp, "cannot create integer field", TCL_STATIC);
					return TCL_ERROR;
				}
			}
			else if (strcmp(type, "double") == 0) {
				if (DBFAddField(shapefile->dbf, name, FTDouble, width, precision) == -1) {
					Tcl_SetResult(interp, "cannot create double field", TCL_STATIC);
					return TCL_ERROR;
				}
			}
			else {
				Tcl_SetResult(interp, "unrecognized field type", TCL_STATIC);
				return TCL_ERROR;
			}
		}
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
	Tcl_CreateObjCommand(interp, cmdName, shapefile_commands, (ClientData)shapefile, shapefile_util_delete);
	Tcl_CreateExitHandler(shapefile_util_exit, (ClientData)shapefile);
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
	
	Tcl_CreateObjCommand(interp, "shapetcl", shapetcl_cmd, NULL, NULL);
	
	return TCL_OK;
}
