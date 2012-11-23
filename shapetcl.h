#ifndef _SHAPETCL_H_
#define _SHAPETCL_H_

typedef struct {
	SHPHandle shp;
	DBFHandle dbf;
	int readonly;
} shapetcl_shapefile;
typedef shapetcl_shapefile * ShapefilePtr;

int util_flagIsPresent(int objc, Tcl_Obj *CONST objv[], const char *flagName);
void shapefile_util_close(ClientData clientData);
void shapefile_util_delete(ClientData clientData);
int shapefile_util_fieldDescription(Tcl_Interp *interp, ShapefilePtr shapefile, int fieldi);
int shapefile_util_coordWrite(Tcl_Interp *interp, ShapefilePtr shapefile, int featureId, Tcl_Obj *coordParts);
int shapefile_util_coordRead(Tcl_Interp *interp, ShapefilePtr shapefile, int featureId, int allCoords, int xyOnly);
int shapefile_util_attrValidate(Tcl_Interp *interp, ShapefilePtr shapefile, Tcl_Obj *attrList);
int shapefile_util_attrWrite(Tcl_Interp *interp, ShapefilePtr shapefile, int recordId, int validate, Tcl_Obj *attrList);
int shapefile_util_attrRead(Tcl_Interp *interp, ShapefilePtr shapefile, int recordId);

#endif
