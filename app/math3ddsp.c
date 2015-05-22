#include "math3ddsp.h"

float my_cos_look[361]; // 1 extra element so we can store 0-360 inclusive
float my_sin_look[361];

//////////////////////////////////////////////////////////////

float32_t VECTOR3D_Length(VECTOR3D_PTR va)
{
// computes the magnitude of a vector, slow

return( (float32_t)sqrtf(va->x*va->x + va->y*va->y + va->z*va->z) );

} // end VECTOR3D_Length

///////////////////////////////////////////////////////////////

void VECTOR3D_Normalize(VECTOR3D_PTR va, VECTOR3D_PTR vn)
{
// normalizes the sent vector and returns the result in vn

VECTOR3D_ZERO(vn);

// compute length
float32_t length = VECTOR3D_Length(va);

// test for zero length vector
// if found return zero vector
if (length < EPSILON_E5) 
   return;

float32_t length_inv = 1.0/length;

// compute normalized version of vector
vn->x = va->x*length_inv;
vn->y = va->y*length_inv;
vn->z = va->z*length_inv;

} // end VECTOR3D_Normalize

///////////////////////////////////////////////////////////////////////////////

float32_t Fast_Distance_3D(float32_t fx, float32_t fy, float32_t fz)
{
	// this function computes the distance from the origin to x,y,z

	int32_t temp;  // used for swaping
	int32_t x,y,z; // used for algorithm

	// make sure values are all positive
	x = fabs(fx) * 1024;
	y = fabs(fy) * 1024;
	z = fabs(fz) * 1024;

	// sort values
	if (y < x) SWAP(x,y,temp)

	if (z < y) SWAP(y,z,temp)

	if (y < x) SWAP(x,y,temp)

	int32_t dist = (z + 11 * (y >> 5) + (x >> 2) );

	// compute distance with 8% error
	return((float32_t)(dist >> 10));

} //

////////////////////////////////////////////////////////////////

// these are the 4D version of the vector functions, they
// assume that the vectors are 3D with a w, so w is left
// out of all the operations

void VECTOR4D_Build(VECTOR4D_PTR init, VECTOR4D_PTR term, VECTOR4D_PTR result)
{
	// build a 4d vector
	result->x = term->x - init->x;
	result->y = term->y - init->y;
	result->z = term->z - init->z;
	result->w = 1;

} // end VECTOR4D_Build

////////////////////////////////////////////////////////////////

void VECTOR4D_Add(VECTOR4D_PTR va, VECTOR4D_PTR vb, VECTOR4D_PTR vsum)
{
	// this function adds va+vb and return it in vsum
	vsum->x = va->x + vb->x;
	vsum->y = va->y + vb->y;
	vsum->z = va->z + vb->z;
	vsum->w = 1;

} // end VECTOR4D_Add

////////////////////////////////////////////////////////////

VECTOR4D VECTOR4D_Add1(VECTOR4D_PTR va, VECTOR4D_PTR vb)
{
	// this function adds va+vb and returns the result on 
	// the stack
	VECTOR4D vsum;

	vsum.x = va->x + vb->x;
	vsum.y = va->y + vb->y;
	vsum.z = va->z + vb->z;
	vsum.w = 1;

	// return result
	return(vsum);

} // end VECTOR4D_Add

////////////////////////////////////////////////////////////

void VECTOR4D_Sub(VECTOR4D_PTR va, VECTOR4D_PTR vb, VECTOR4D_PTR vdiff)
{
	// this function subtracts va-vb and return it in vdiff
	// the stack
	vdiff->x = va->x - vb->x;
	vdiff->y = va->y - vb->y;
	vdiff->z = va->z - vb->z;
	vdiff->w = 1;

} // end VECTOR4D_Sub

////////////////////////////////////////////////////////////

VECTOR4D VECTOR4D_Sub1(VECTOR4D_PTR va, VECTOR4D_PTR vb)
{
	// this function subtracts va-vb and returns the result on 
	// the stack
	VECTOR4D vdiff;

	vdiff.x = va->x - vb->x;
	vdiff.y = va->y - vb->y;
	vdiff.z = va->z - vb->z;
	vdiff.w = 1;

	// return result
	return(vdiff);                      

} // end VECTOR4D_Sub

////////////////////////////////////////////////////////////

void VECTOR4D_Scale(float32_t k, VECTOR4D_PTR va)
{
	// this function scales a vector by the constant k,
	// in place , note w is left unchanged

	// multiply each component by scaling factor
	va->x*=k;
	va->y*=k;
	va->z*=k;
	va->w = 1;

} // end VECTOR4D_Scale

/////////////////////////////////////////////////////////////

void VECTOR4D_Scale1(float32_t k, VECTOR4D_PTR va, VECTOR4D_PTR vscaled)
{
	// this function scales a vector by the constant k,
	// leaves the original unchanged, and returns the result
	// in vres as well as on the stack

	// multiply each component by scaling factor
	vscaled->x = k*va->x;
	vscaled->y = k*va->y;
	vscaled->z = k*va->z;
	vscaled->w = 1;

} // end VECTOR4D_Scale

//////////////////////////////////////////////////////////////

float32_t VECTOR4D_Dot(VECTOR4D_PTR va, VECTOR4D_PTR vb)
{
	// computes the dot product between va and vb
	return( (va->x * vb->x) + (va->y * vb->y) + (va->z * vb->z) );
} // end VECTOR4D_DOT

/////////////////////////////////////////////////////////////

void VECTOR4D_Cross(VECTOR4D_PTR va, 
                    VECTOR4D_PTR vb,
                    VECTOR4D_PTR vn)
{
	// this function computes the cross product between va and vb
	// and returns the vector that is perpendicular to each in vn

	vn->x =  ( (va->y * vb->z) - (va->z * vb->y) );
	vn->y = -( (va->x * vb->z) - (va->z * vb->x) );
	vn->z =  ( (va->x * vb->y) - (va->y * vb->x) ); 
	vn->w = 1;

} // end VECTOR4D_Cross

/////////////////////////////////////////////////////////////

VECTOR4D VECTOR4D_Cross1(VECTOR4D_PTR va, VECTOR4D_PTR vb)
{
	// this function computes the cross product between va and vb
	// and returns the vector that is perpendicular to each

	VECTOR4D vn;

	vn.x =  ( (va->y * vb->z) - (va->z * vb->y) );
	vn.y = -( (va->x * vb->z) - (va->z * vb->x) );
	vn.z =  ( (va->x * vb->y) - (va->y * vb->x) ); 
	vn.w = 1;

	// return result
	return(vn);

} // end VECTOR4D_Cross

//////////////////////////////////////////////////////////////

float32_t VECTOR4D_Length(VECTOR4D_PTR va)
{
	// computes the magnitude of a vector, slow

	return(sqrtf(va->x*va->x + va->y*va->y + va->z*va->z) );

} // end VECTOR4D_Length

///////////////////////////////////////////////////////////////

float32_t VECTOR4D_Length_Fast(VECTOR4D_PTR va)
{
	// computes the magnitude of a vector using an approximation
	// very fast
	return( Fast_Distance_3D(va->x, va->y, va->z) );

} // end VECTOR4D_Length_Fast

///////////////////////////////////////////////////////////////

void VECTOR4D_Normalize(VECTOR4D_PTR va)
{
	// normalizes the sent vector and returns the result

	// compute length
	float32_t length = sqrtf(va->x*va->x + va->y*va->y + va->z*va->z);

	// test for zero length vector
	// if found return zero vector
	if (length < EPSILON_E5) 
	   return;

	float32_t length_inv = 1.0/length;

	// compute normalized version of vector
	va->x*=length_inv;
	va->y*=length_inv;
	va->z*=length_inv;
	va->w = 1;

} // end VECTOR4D_Normalize

///////////////////////////////////////////////////////////////

void VECTOR4D_Normalize1(VECTOR4D_PTR va, VECTOR4D_PTR vn)
{
	// normalizes the sent vector and returns the result in vn

	VECTOR4D_ZERO(vn);

	// compute length
	float32_t length = sqrt(va->x*va->x + va->y*va->y + va->z*va->z);

	// test for zero length vector
	// if found return zero vector
	if (length < EPSILON_E5) 
	   return;

	float32_t length_inv = 1.0/length;

	// compute normalized version of vector
	vn->x = va->x*length_inv;
	vn->y = va->y*length_inv;
	vn->z = va->z*length_inv;
	vn->w = 1;

} // end VECTOR4D_Normalize

///////////////////////////////////////////////////////////////

float32_t VECTOR4D_CosTh(VECTOR4D_PTR va, VECTOR4D_PTR vb)
{
	// this function returns the cosine of the angle between
	// two vectors. Note, we could compute the actual angle,
	// many many times, in further calcs we will want ultimately
	// compute cos of the angle, so why not just leave it!
	return(VECTOR4D_Dot(va,vb)/(VECTOR4D_Length(va)*VECTOR4D_Length(vb)));

} // end VECTOR4D_CosTh



///////////////////////////////////////////////////////////////

void PLANE3D_Init(PLANE3D_PTR plane, POINT3D_PTR p0, 
                  VECTOR3D_PTR normal, int32_t normalize)
{
// this function initializes a 3d plane

// copy the point
POINT3D_COPY(&plane->p0, p0);

// if normalize is 1 then the normal is made into a unit vector
if (!normalize)
   VECTOR3D_COPY(&plane->n, normal);
else
   {
   // make normal into unit vector
   VECTOR3D_Normalize(normal,&plane->n);
   } // end else

} // end PLANE3D_Init

void Build_Sin_Cos_Tables(void)
{
	// create sin/cos lookup table
	// note the creation of one extra element; 360
	// this helps with logic in using the tables

	// generate the tables 0 - 360 inclusive
	for (int ang = 0; ang <= 360; ang++)
    {
		// convert ang to radians
		float theta = (float)ang*PI/(float)180;

		// insert next entry into table
		my_cos_look[ang] = cos(theta);
		my_sin_look[ang] = sin(theta);

    } // end for ang

} // end Build_Sin_Cos_Tables

float Fast_Sin(float theta)
{
	// this function uses the sin_look[] lookup table, but
	// has logic to handle negative angles as well as fractional
	// angles via interpolation, use this for a more robust
	// sin computation that the blind lookup, but with with
	// a slight hit in speed

	// convert angle to 0-359
	theta = fmodf(theta,360);

	// make angle positive
	if (theta < 0) theta+=360.0;

	// compute floor of theta and fractional part to interpolate
	int theta_int    = (int)theta;
	float theta_frac = theta - theta_int;

	// now compute the value of sin(angle) using the lookup tables
	// and interpolating the fractional part, note that if theta_int
	// is equal to 359 then theta_int+1=360, but this is fine since the
	// table was made with the entries 0-360 inclusive
	return(my_sin_look[theta_int] + 
		   theta_frac*(my_sin_look[theta_int+1] - my_sin_look[theta_int]));

} // end Fast_Sin

///////////////////////////////////////////////////////////////

float Fast_Cos(float theta)
{
	// this function uses the cos_look[] lookup table, but
	// has logic to handle negative angles as well as fractional
	// angles via interpolation, use this for a more robust
	// cos computation that the blind lookup, but with with
	// a slight hit in speed

	// convert angle to 0-359
	theta = fmodf(theta,360);

	// make angle positive
	if (theta < 0) theta+=360.0;

	// compute floor of theta and fractional part to interpolate
	int theta_int    = (int)theta;
	float theta_frac = theta - theta_int;

	// now compute the value of sin(angle) using the lookup tables
	// and interpolating the fractional part, note that if theta_int
	// is equal to 359 then theta_int+1=360, but this is fine since the
	// table was made with the entries 0-360 inclusive
	return(my_cos_look[theta_int] + 
		   theta_frac*(my_cos_look[theta_int+1] - my_cos_look[theta_int]));

} // end Fast_Cos

