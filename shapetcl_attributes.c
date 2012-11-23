#include <stdio.h>
#include <string.h>
#include <tcl.h>
#include "shapefil.h"

#include "shapetcl.h"
#include "shapetcl_attributes.h"

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
