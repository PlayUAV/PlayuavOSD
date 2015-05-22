#ifndef __MATH3D_H_
#define __MATH3D_H_

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

// a 2D vertex
typedef struct VERTEX2DF_TYP
{
	float32_t x,y; // the vertex
} VERTEX2DF, *VERTEX2DF_PTR;

// note that 1x2 has the same memory layout as a VERTEX2DF, hence we
// can use the matrix function written for a MATRIX1X2 to multiply a 
// VERTEX2DF by casting
typedef struct MATRIX1X2_TYP
{
	float32_t M[2]; // data storage
} MATRIX1X2, *MATRIX1X2_PTR;


typedef struct MATRIX3X2_TYP
{
	float32_t M[3][2]; // data storage
} MATRIX3X2, *MATRIX3X2_PTR;

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

// 1x4 matrix /////////////////////////////////////////////
typedef struct MATRIX1X4_TYP
{
union
    {
    float32_t M[4]; // array indexed data storage

    // storage in row major form with explicit names
    struct
         {
         float32_t M00, M01, M02, M03;
         }; // end explicit names

     }; // end union
} MATRIX1X4, *MATRIX1X4_PTR;

// 4x4 matrix /////////////////////////////////////////////
typedef struct MATRIX4X4_TYP
{
	union
		{
		float32_t M[4][4]; // array indexed data storage

		// storage in row major form with explicit names
		struct
			 {
			 float32_t M00, M01, M02, M03;
			 float32_t M10, M11, M12, M13;
			 float32_t M20, M21, M22, M23;
			 float32_t M30, M31, M32, M33;
			 }; // end explicit names

		}; // end union

} MATRIX4X4, *MATRIX4X4_PTR;

// 4x3 matrix /////////////////////////////////////////////
typedef struct MATRIX4X3_TYP
{
union
    {
    float32_t M[4][3]; // array indexed data storage

    // storage in row major form with explicit names
    struct
         {
         float32_t M00, M01, M02;
         float32_t M10, M11, M12;
         float32_t M20, M21, M22;
         float32_t M30, M31, M32;
         }; // end explicit names

    }; // end union

} MATRIX4X3, *MATRIX4X3_PTR;


// used to compute the min and max of two expresions
#define MIN(a, b)  (((a) < (b)) ? (a) : (b)) 
#define MAX(a, b)  (((a) > (b)) ? (a) : (b)) 

// bit manipulation macros
#define SET_BIT_MATH3D(word,bit_flag)   ((word)=((word) | (bit_flag)))
#define RESET_BIT_MATH3D(word,bit_flag) ((word)=((word) & (~bit_flag)))

// used for swapping algorithm
#define SWAP3D(a,b,t) {t=a; a=b; b=t;}
#define EPSILON_E5 (float32_t)(1E-5)
float32_t Fast_Distance_3D(float32_t fx, float32_t fy, float32_t fz);

static inline void VECTOR2D_INITXYZ(VERTEX2DF_PTR v, float32_t x, float32_t y) 
{(v)->x = (x); (v)->y = (y);}

static inline void VECTOR2D_COPY(VERTEX2DF_PTR vdst, VERTEX2DF_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y; }

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

static inline void VECTOR4D_INITXYZW(VECTOR4D_PTR v, float32_t x,float32_t y,float32_t z,float32_t w) 
{(v)->x = (x); (v)->y = (y); (v)->z = (z); (v)->w = (w);}

// used to convert from 4D homogenous to 4D non-homogenous
static inline void VECTOR4D_DIV_BY_W(VECTOR4D_PTR v) 
{(v)->x/=(v)->w; (v)->y/=(v)->w; (v)->z/=(v)->w;  }

// 3d vector functions
float32_t VECTOR3D_Length(VECTOR3D_PTR va);
void VECTOR3D_Normalize(VECTOR3D_PTR va, VECTOR3D_PTR vn);

static inline int Mat_Init_3X2(MATRIX3X2_PTR ma, 
                        float m00, float m01,
                        float m10, float m11,
                        float m20, float m21)
{
	// this function fills a 3x2 matrix with the sent data in row major form
	ma->M[0][0] = m00; ma->M[0][1] = m01; 
	ma->M[1][0] = m10; ma->M[1][1] = m11; 
	ma->M[2][0] = m20; ma->M[2][1] = m21; 

	// return success
	return(1);

} // end Mat_Init_3X2

int Mat_Mul1X2_3X2(MATRIX1X2_PTR ma, 
                   MATRIX3X2_PTR mb,
                   MATRIX1X2_PTR mprod);

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
const static MATRIX4X4 IMAT_4X4 = {1,0,0,0, 
                            0,1,0,0, 
                            0,0,1,0, 
                            0,0,0,1};
// macros to set the identity matrix
#define MAT_IDENTITY_4X4(m) {memcpy((void *)(m), (void *)&IMAT_4X4, sizeof(MATRIX4X4));}
#define MAT_COPY_4X4(src_mat, dest_mat) {memcpy((void *)(dest_mat), (void *)(src_mat), sizeof(MATRIX4X4) ); }

// 4x4 matrix functions
void Mat_Add_4X4(MATRIX4X4_PTR ma, MATRIX4X4_PTR mb, MATRIX4X4_PTR msum);
void Mat_Mul_4X4(MATRIX4X4_PTR ma, MATRIX4X4_PTR mb, MATRIX4X4_PTR mprod);
void Mat_Mul_1X4_4X4(MATRIX1X4_PTR ma, MATRIX4X4_PTR mb, MATRIX1X4_PTR mprod);
void Mat_Mul_VECTOR3D_4X4(VECTOR3D_PTR  va, MATRIX4X4_PTR mb, VECTOR3D_PTR  vprod);
void Mat_Mul_VECTOR3D_4X3(VECTOR3D_PTR  va, MATRIX4X3_PTR mb, VECTOR3D_PTR  vprod);
void Mat_Mul_VECTOR4D_4X4(VECTOR4D_PTR  va, MATRIX4X4_PTR mb, VECTOR4D_PTR  vprod);
void Mat_Mul_VECTOR4D_4X3(VECTOR4D_PTR  va, MATRIX4X4_PTR mb, VECTOR4D_PTR  vprod);
int32_t Mat_Inverse_4X4(MATRIX4X4_PTR m, MATRIX4X4_PTR mi);
void Mat_Init_4X4(MATRIX4X4_PTR ma, 
                        float32_t m00, float32_t m01, float32_t m02, float32_t m03,
                        float32_t m10, float32_t m11, float32_t m12, float32_t m13,
                        float32_t m20, float32_t m21, float32_t m22, float32_t m23,
                        float32_t m30, float32_t m31, float32_t m32, float32_t m33);
	
// 3d plane functions
void PLANE3D_Init(PLANE3D_PTR plane, POINT3D_PTR p0, 
                         VECTOR3D_PTR normal, int32_t normalize);
#endif //__MATH3D_H_
