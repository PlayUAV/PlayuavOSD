#ifndef __MATH3DDSP_H_
#define __MATH3DDSP_H_

#pragma anon_unions

#include "arm_math.h" 
#include "math_helper.h" 

#define XPI	3.141592653589793f					//3.14159265358979323846
// some math macros
#define DEG_TO_RAD(ang) ((ang)*XPI/180.0)
#define RAD_TO_DEG(rads) ((rads)*180.0/XPI)

// storage for our lookup tables

void Build_Sin_Cos_Tables(void);
float Fast_Sin(float theta);
float Fast_Cos(float theta);

// 3D vector, point without the w ////////////////////////
typedef struct VECTOR3D_TYP
{
	union
		{
		float32_t M[3]; // array indexed storage

		// explicit names
		struct
			 {
			 float32_t x,y,z;
			 }; // end struct

		}; // end union

} VECTOR3D, POINT3D, *VECTOR3D_PTR, *POINT3D_PTR;

// 4D homogenous vector, point with w ////////////////////
typedef struct VECTOR4D_TYP
{
	union
		{
		float32_t M[4]; // array indexed storage

		// explicit names
		struct
			 {
			 float32_t x,y,z,w;
			 }; // end struct
		}; // end union

} VECTOR4D, POINT4D, *VECTOR4D_PTR, *POINT4D_PTR;

// 3D plane ///////////////////////////////////////////////////
typedef struct PLANE3D_TYP
{
	POINT3D p0; // point on the plane
	VECTOR3D n; // normal to the plane (not necessarily a unit vector)
} PLANE3D, *PLANE3D_PTR;



// used to compute the min and max of two expresions
#define MIN(a, b)  (((a) < (b)) ? (a) : (b)) 
#define MAX(a, b)  (((a) > (b)) ? (a) : (b)) 

// bit manipulation macros
#define SET_BIT(word,bit_flag)   ((word)=((word) | (bit_flag)))
#define RESET_BIT(word,bit_flag) ((word)=((word) & (~bit_flag)))

// used for swapping algorithm
#define SWAP(a,b,t) {t=a; a=b; b=t;}
#define EPSILON_E5 (float32_t)(1E-5)
float32_t Fast_Distance_3D(float32_t fx, float32_t fy, float32_t fz);

static inline void VECTOR3D_ZERO(VECTOR3D_PTR v) 
{(v)->x = (v)->y = (v)->z = 0.0;}

static inline void POINT3D_COPY(POINT3D_PTR vdst, POINT3D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  (vdst)->z = (vsrc)->z; }

static inline void VECTOR3D_COPY(VECTOR3D_PTR vdst, VECTOR3D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  (vdst)->z = (vsrc)->z; }

static inline void VECTOR3D_INITXYZ(VECTOR3D_PTR v, float32_t x, float32_t y, float32_t z) 
{(v)->x = (x); (v)->y = (y); (v)->z = (z);}

static inline void VECTOR4D_ZERO(VECTOR4D_PTR v) 
{(v)->x = (v)->y = (v)->z = 0.0; (v)->w = 1.0;}

static inline void VECTOR4D_COPY(VECTOR4D_PTR vdst, VECTOR4D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  
(vdst)->z = (vsrc)->z; (vdst)->w = (vsrc)->w;  }

static inline void VECTOR4D_INITXYZ(VECTOR4D_PTR v, float32_t x,float32_t y,float32_t z) 
{(v)->x = (x); (v)->y = (y); (v)->z = (z); (v)->w = 1.0;}

// used to convert from 4D homogenous to 4D non-homogenous
static inline void VECTOR4D_DIV_BY_W(VECTOR4D_PTR v) 
{(v)->x/=(v)->w; (v)->y/=(v)->w; (v)->z/=(v)->w;  }

// 3d vector functions
float32_t VECTOR3D_Length(VECTOR3D_PTR va);
void VECTOR3D_Normalize(VECTOR3D_PTR va, VECTOR3D_PTR vn);

// 4d vector functions
void VECTOR4D_Add(VECTOR4D_PTR va, VECTOR4D_PTR vb, VECTOR4D_PTR vsum);
VECTOR4D VECTOR4D_Add1(VECTOR4D_PTR va, VECTOR4D_PTR vb);
void VECTOR4D_Sub(VECTOR4D_PTR va, VECTOR4D_PTR vb, VECTOR4D_PTR vdiff);
VECTOR4D VECTOR4D_Sub1(VECTOR4D_PTR va, VECTOR4D_PTR vb);
void VECTOR4D_Scale(float32_t k, VECTOR4D_PTR va);
void VECTOR4D_Scale1(float32_t k, VECTOR4D_PTR va, VECTOR4D_PTR vscaled);
float32_t VECTOR4D_Dot(VECTOR4D_PTR va, VECTOR4D_PTR vb);
void VECTOR4D_Cross(VECTOR4D_PTR va,VECTOR4D_PTR vb,VECTOR4D_PTR vn);
VECTOR4D VECTOR4D_Cross1(VECTOR4D_PTR va, VECTOR4D_PTR vb);
float32_t VECTOR4D_Length(VECTOR4D_PTR va);
float32_t VECTOR4D_Length_Fast(VECTOR4D_PTR va);
void VECTOR4D_Normalize(VECTOR4D_PTR va);
void VECTOR4D_Normalize1(VECTOR4D_PTR va, VECTOR4D_PTR vn);
void VECTOR4D_Build(VECTOR4D_PTR init, VECTOR4D_PTR term, VECTOR4D_PTR result);
float32_t VECTOR4D_CosTh(VECTOR4D_PTR va, VECTOR4D_PTR vb);

// 4x4 identity matrix
const static float32_t IMAT_4X4[16] = { 1,0,0,0, 
										0,1,0,0, 
										0,0,1,0, 
										0,0,0,1};
static inline void MAT_IDENTITY_4X4(arm_matrix_instance_f32* m)
 {arm_mat_init_f32(m, 4, 4, (float32_t *)IMAT_4X4);}
static inline void MAT_COPY_4X4(arm_matrix_instance_f32* src_mat, arm_matrix_instance_f32* dest_mat) 
{  dest_mat->numRows = 4;  dest_mat->numCols = 4; memcpy((void *)(dest_mat->pData), (void *)(src_mat->pData), sizeof(IMAT_4X4) );}

// 3d plane functions
void PLANE3D_Init(PLANE3D_PTR plane, POINT3D_PTR p0, 
                         VECTOR3D_PTR normal, int32_t normalize);
#endif //__MATH3DDSP_H_
