#include <stdio.h>
#include <string.h>
#include <tcl.h>
#include "shapefil.h"

#include "shapetcl.h"
#include "shapetcl_coords.h"

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
