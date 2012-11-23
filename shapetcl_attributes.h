#ifndef _SHAPETCL_ATTRIBUTES_H_
#define _SHAPETCL_ATTRIBUTES_H_

int shapefile_util_attrValidate(Tcl_Interp *interp, ShapefilePtr shapefile, Tcl_Obj *attrList);
int shapefile_util_attrWrite(Tcl_Interp *interp, ShapefilePtr shapefile, int recordId, int validate, Tcl_Obj *attrList);
int shapefile_util_attrRead(Tcl_Interp *interp, ShapefilePtr shapefile, int recordId);
int shapefile_cmd_attributes(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

#endif
