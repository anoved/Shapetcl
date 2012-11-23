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

