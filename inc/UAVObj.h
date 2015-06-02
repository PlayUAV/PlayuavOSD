#ifndef __UAVOBJ_H_
#define __UAVOBJ_H_

#include "m3dlib.h"
#include "m2dlib.h"

extern CAM4DV1    	cam;     // the single camera
extern OBJECT4DV1 	uav3D;   // used to hold our cube mesh  
extern POLYGON2D 	uav2D; 
extern POLYGON2D 	rollscale2D; 

void cam3D_init(void);
void uav3D_init(void);


void uav2D_init(void);

#endif //__UAVOBJ_H_