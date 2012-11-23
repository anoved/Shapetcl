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

#endif
