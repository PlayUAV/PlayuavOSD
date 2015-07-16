#ifndef M3DLIB_H__
#define M3DLIB_H__

#include "math3d.h"

// DEFINES ////////////////////////////////////////////////////


// states of polygons and faces
#define POLY4DV1_STATE_ACTIVE             0x0001
#define POLY4DV1_STATE_CLIPPED            0x0002
#define POLY4DV1_STATE_BACKFACE           0x0004

// defines for objects version 1
#define OBJECT4DV1_MAX_VERTICES           32  // 64
#define OBJECT4DV1_MAX_POLYS              32 // 128

// states for objects
#define OBJECT4DV1_STATE_ACTIVE           0x0001
#define OBJECT4DV1_STATE_VISIBLE          0x0002 
#define OBJECT4DV1_STATE_CULLED           0x0004

// attributes for objects

// render list defines
#define RENDERLIST4DV1_MAX_POLYS          10// 16384

// transformation control flags
#define TRANSFORM_LOCAL_ONLY       0  // perform the transformation in place on the
                                      // local/world vertex list 
#define TRANSFORM_TRANS_ONLY       1  // perfrom the transformation in place on the 
                                      // "transformed" vertex list

#define TRANSFORM_LOCAL_TO_TRANS   2  // perform the transformation to the local
                                      // vertex list, but store the results in the
                                      // transformed vertex list

//// general culling flags
//#define CULL_OBJECT_X_PLANE           0x0001 // cull on the x clipping planes
//#define CULL_OBJECT_Y_PLANE           0x0002 // cull on the y clipping planes
//#define CULL_OBJECT_Z_PLANE           0x0004 // cull on the z clipping planes
//#define CULL_OBJECT_XYZ_PLANES        (CULL_OBJECT_X_PLANE | CULL_OBJECT_Y_PLANE | CULL_OBJECT_Z_PLANE)

//// defines for camera rotation sequences
//#define CAM_ROT_SEQ_XYZ  0
//#define CAM_ROT_SEQ_YXZ  1
//#define CAM_ROT_SEQ_XZY  2
//#define CAM_ROT_SEQ_YZX  3
//#define CAM_ROT_SEQ_ZYX  4
//#define CAM_ROT_SEQ_ZXY  5

//// defines for special types of camera projections
//#define CAM_PROJ_NORMALIZED        0x0001
//#define CAM_PROJ_SCREEN            0x0002
//#define CAM_PROJ_FOV90             0x0004

#define CAM_MODEL_EULER            0x0008
#define CAM_MODEL_UVN              0x0010

//#define UVN_MODE_SIMPLE            0 
//#define UVN_MODE_SPHERICAL         1

	
// TYPES //////////////////////////////////////////////////////

// a polygon based on an external vertex list
typedef struct POLY4DV1_TYP
{
	int32_t state;    // state information
	int32_t attr;     // physical attributes of polygon
	int32_t color;    // color of polygon

	POINT4D_PTR vlist; // the vertex list itself
	int32_t vert[3];       // the indices into the vertex list

} POLY4DV1, *POLY4DV1_PTR;

// a self contained polygon used for the render list
typedef struct POLYF4DV1_TYP
{
	int32_t state;    // state information
	int32_t attr;     // physical attributes of polygon
	int32_t color;    // color of polygon

	POINT4D vlist[3];  // the vertices of this triangle
	POINT4D tvlist[3]; // the vertices after transformation if needed

	struct POLYF4DV1_TYP *next; // pointer to next polygon in list??
	struct POLYF4DV1_TYP *prev; // pointer to previous polygon in list??

} POLYF4DV1, *POLYF4DV1_PTR;

// an object based on a vertex list and list of polygons
typedef struct OBJECT4DV1_TYP
{
	int32_t  id;           // numeric id of this object
//	char name[64];     // ASCII name of object just for kicks
	int32_t  state;        // state of object
	int32_t  attr;         // attributes of object
//	float avg_radius;  // average radius of object used for collision detection
//	float max_radius;  // maximum radius of object

	POINT4D world_pos;  // position of object in world

	VECTOR4D dir;       // rotation angles of object in local
						// cords or unit direction vector user defined???

//	VECTOR4D ux,uy,uz;  // local axes to track full orientation
//						// this is updated automatically during
//						// rotation calls

	int32_t num_vertices;   // number of vertices of this object

	POINT4D vlist_local[OBJECT4DV1_MAX_VERTICES]; // array of local vertices
	POINT4D vlist_trans[OBJECT4DV1_MAX_VERTICES]; // array of transformed vertices

	int32_t num_polys;        // number of polygons in object mesh
	POLY4DV1 plist[OBJECT4DV1_MAX_POLYS];  // array of polygons

} OBJECT4DV1, *OBJECT4DV1_PTR;


// camera version 1
typedef struct CAM4DV1_TYP
{
	int32_t state;      // state of camera
	int32_t attr;       // camera attributes

	POINT4D pos;    // world position of camera used by both camera models

	VECTOR4D dir;   // angles or look at direction of camera for simple 
					// euler camera models, elevation and heading for
					// uvn model

//	VECTOR4D u;     // extra vectors to track the camera orientation
//	VECTOR4D v;     // for more complex UVN camera model
//	VECTOR4D n;        

//	VECTOR4D target; // look at target

	float view_dist;  // focal length 

	float fov;          // field of view for both horizontal and vertical axes

	// 3d clipping planes
	// if view volume is NOT 90 degree then general 3d clipping
	// must be employed
	float near_clip_z;     // near z=constant clipping plane
	float far_clip_z;      // far z=constant clipping plane

	PLANE3D rt_clip_plane;  // the right clipping plane
	PLANE3D lt_clip_plane;  // the left clipping plane
	PLANE3D tp_clip_plane;  // the top clipping plane
	PLANE3D bt_clip_plane;  // the bottom clipping plane                        

	float viewplane_width;     // width and height of view plane to project onto
	float viewplane_height;    // usually 2x2 for normalized projection or 
							   // the exact same size as the viewport or screen window

	// remember screen and viewport are synonomous 
	float viewport_width;     // size of screen/viewport
	float viewport_height;
	float viewport_center_x;  // center of view port (final image destination)
	float viewport_center_y;

	// aspect ratio
	float aspect_ratio;

	// these matrices are not necessarily needed based on the method of
	// transformation, for example, a manual perspective or screen transform
	// and or a concatenated perspective/screen, however, having these 
	// matrices give us more flexibility         

	MATRIX4X4 mcam;   // storage for the world to camera transform matrix
	MATRIX4X4 mper;   // storage for the camera to perspective transform matrix
	MATRIX4X4 mscr;   // storage for the perspective to screen transform matrix

} CAM4DV1, *CAM4DV1_PTR;


void Reset_OBJECT4DV1(OBJECT4DV1_PTR obj);

void Translate_OBJECT4DV1(OBJECT4DV1_PTR obj, VECTOR4D_PTR vt);

void Scale_OBJECT4DV1(OBJECT4DV1_PTR obj, VECTOR4D_PTR vs);

void Transform_OBJECT4DV1(OBJECT4DV1_PTR obj, MATRIX4X4_PTR mt,   
                          int coord_select);

void Build_XYZ_Rotation_MATRIX4X4(float theta_x, float theta_y, float theta_z,
                                  MATRIX4X4_PTR mrot);

void Transform_To_World_OBJECT4DV1(OBJECT4DV1_PTR obj, MATRIX4X4_PTR mt);

void Transform_To_Screen_OBJECT4DV1(OBJECT4DV1_PTR obj, CAM4DV1_PTR cam);

void Build_CAM4DV1_Matrix_Euler(CAM4DV1_PTR cam);


void Init_CAM4DV1(CAM4DV1_PTR cam, int32_t attr, POINT4D_PTR cam_pos, 
                  VECTOR4D_PTR cam_dir, VECTOR4D_PTR cam_target,
                  float near_clip_z, float far_clip_z, float fov, 
                  float viewport_width,  float viewport_height);

void Adjust_Viewport_CAM4DV1(CAM4DV1_PTR cam,       // the camera object
                  float viewport_width,  // size of final screen viewport
                  float viewport_height);




#endif
