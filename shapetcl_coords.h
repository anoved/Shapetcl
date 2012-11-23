#ifndef _SHAPETCL_COORDS_H
#define _SHAPETCL_COORDS_H

int shapefile_util_coordWrite(Tcl_Interp *interp, ShapefilePtr shapefile, int featureId, Tcl_Obj *coordParts);
int shapefile_util_coordRead(Tcl_Interp *interp, ShapefilePtr shapefile, int featureId, int allCoords, int xyOnly);
int shapefile_cmd_coords(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

#endif
