/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
/*
 * With Grateful Acknowledgements to the books writen by Andre LaMothe:
 * <Tricks of the 3D Game Programming Gurus-Advanced 3D Graphics and Rasterization>
 * <Tricks of the Windows Game Programming Gurus>
 */
#include "m3dlib.h"

////////////////////////////////////////////////////////////////
void Reset_OBJECT4DV1(OBJECT4DV1_PTR obj) {
    // this function resets the sent object and redies it for
    // transformations, basically just resets the culled, clipped and
    // backface flags, but here's where you would add stuff
    // to ready any object for the pipeline
    // the object is valid, let's rip it apart polygon by polygon

    // reset object's culled flag
    RESET_BIT_MATH3D(obj->state, OBJECT4DV1_STATE_CULLED);

    // now the clipped and backface flags for the polygons
    for (int32_t poly = 0; poly < obj->num_polys; poly++) {
        // acquire polygon
        POLY4DV1_PTR curr_poly = &obj->plist[poly];

        // first is this polygon even visible?
        if (!(curr_poly->state & POLY4DV1_STATE_ACTIVE))
            continue; // move onto next poly

        // reset clipped and backface flags
        RESET_BIT_MATH3D(curr_poly->state, POLY4DV1_STATE_CLIPPED);
        RESET_BIT_MATH3D(curr_poly->state, POLY4DV1_STATE_BACKFACE);

    } // end for poly

} // end Reset_OBJECT4DV1

void Translate_OBJECT4DV1(OBJECT4DV1_PTR obj, VECTOR4D_PTR vt) {
    // NOTE: Not matrix based
    // this function translates an object without matrices,
    // simply updates the world_pos
    VECTOR4D_Add(&obj->world_pos, vt, &obj->world_pos);

} // end Translate_OBJECT4DV1

/////////////////////////////////////////////////////////////

void Scale_OBJECT4DV1(OBJECT4DV1_PTR obj, VECTOR4D_PTR vs) {
    // NOTE: Not matrix based
    // this function scales and object without matrices
    // modifies the object's local vertex list
    // additionally the radii is updated for the object

    // for each vertex in the mesh scale the local coordinates by
    // vs on a componentwise basis, that is, sx, sy, sz
    for (int vertex = 0; vertex < obj->num_vertices; vertex++) {
        obj->vlist_local[vertex].x *= vs->x;
        obj->vlist_local[vertex].y *= vs->y;
        obj->vlist_local[vertex].z *= vs->z;
        // leave w unchanged, always equal to 1

    } // end for vertex

} // end Scale_OBJECT4DV1

///////////////////////////////////////////////////////////

void Transform_OBJECT4DV1(OBJECT4DV1_PTR obj, // object to transform
        MATRIX4X4_PTR mt,   // transformation matrix
        int coord_select)   // selects coords to transform
{
// this function simply transforms all of the vertices in the local or trans
// array by the sent matrix

// what coordinates should be transformed?
    switch (coord_select) {
    case TRANSFORM_LOCAL_ONLY: {
        // transform each local/model vertex of the object mesh in place
        for (int vertex = 0; vertex < obj->num_vertices; vertex++) {
            POINT4D presult; // hold result of each transformation

            // transform point
            Mat_Mul_VECTOR4D_4X4(&obj->vlist_local[vertex], mt, &presult);

            // store result back
            VECTOR4D_COPY(&obj->vlist_local[vertex], &presult);
        } // end for index
    }
        break;

    case TRANSFORM_TRANS_ONLY: {
        // transform each "transformed" vertex of the object mesh in place
        // remember, the idea of the vlist_trans[] array is to accumulate
        // transformations
        for (int vertex = 0; vertex < obj->num_vertices; vertex++) {
            POINT4D presult; // hold result of each transformation

            // transform point
            Mat_Mul_VECTOR4D_4X4(&obj->vlist_trans[vertex], mt, &presult);

            // store result back
            VECTOR4D_COPY(&obj->vlist_trans[vertex], &presult);
        } // end for index

    }
        break;

    case TRANSFORM_LOCAL_TO_TRANS: {
        // transform each local/model vertex of the object mesh and store result
        // in "transformed" vertex list
        for (int vertex = 0; vertex < obj->num_vertices; vertex++) {
            // transform point
            Mat_Mul_VECTOR4D_4X4(&obj->vlist_local[vertex], mt,
                    &obj->vlist_trans[vertex]);

        } // end for index
    }
        break;

    default:
        break;

    } // end switch

} // end Transform_OBJECT4DV1

///////////////////////////////////////////////////////////////
void Build_XYZ_Rotation_MATRIX4X4(float theta_x, // euler angles
        float theta_y, float theta_z, MATRIX4X4_PTR mrot) // output
{
    // this helper function takes a set if euler angles and computes
    // a rotation matrix from them, usefull for object and camera
    // work, also  we will do a little testing in the function to determine
    // the rotations that need to be performed, since there's no
    // reason to perform extra matrix multiplies if the angles are
    // zero!

    MATRIX4X4 mx, my, mz, mtmp;       // working matrices
    float sin_theta = 0, cos_theta = 0;   // used to initialize matrices
    int32_t rot_seq = 0;                  // 1 for x, 2 for y, 4 for z

    // step 0: fill in with identity matrix
    MAT_IDENTITY_4X4(mrot);

    // step 1: based on zero and non-zero rotation angles, determine
    // rotation sequence
    if (fabs(theta_x) > EPSILON_E5) // x
        rot_seq = rot_seq | 1;

    if (fabs(theta_y) > EPSILON_E5) // y
        rot_seq = rot_seq | 2;

    if (fabs(theta_z) > EPSILON_E5) // z
        rot_seq = rot_seq | 4;

    // now case on sequence
    switch (rot_seq) {
    case 0: // no rotation
    {
        // what a waste!
        return;
    }
        break;

    case 1: // x rotation
    {
        // compute the sine and cosine of the angle
        cos_theta = Fast_Cos(theta_x);
        sin_theta = Fast_Sin(theta_x);

        // set the matrix up
        Mat_Init_4X4(&mx, 1, 0, 0, 0, 0, cos_theta, sin_theta, 0, 0, -sin_theta,
                cos_theta, 0, 0, 0, 0, 1);

        // that's it, copy to output matrix
        MAT_COPY_4X4(&mx, mrot);
        return;

    }
        break;

    case 2: // y rotation
    {
        // compute the sine and cosine of the angle
        cos_theta = Fast_Cos(theta_y);
        sin_theta = Fast_Sin(theta_y);

        // set the matrix up
        Mat_Init_4X4(&my, cos_theta, 0, -sin_theta, 0, 0, 1, 0, 0, sin_theta, 0,
                cos_theta, 0, 0, 0, 0, 1);

        // that's it, copy to output matrix
        MAT_COPY_4X4(&my, mrot);
        return;

    }
        break;

    case 3: // xy rotation
    {
        // compute the sine and cosine of the angle for x
        cos_theta = Fast_Cos(theta_x);
        sin_theta = Fast_Sin(theta_x);

        // set the matrix up
        Mat_Init_4X4(&mx, 1, 0, 0, 0, 0, cos_theta, sin_theta, 0, 0, -sin_theta,
                cos_theta, 0, 0, 0, 0, 1);

        // compute the sine and cosine of the angle for y
        cos_theta = Fast_Cos(theta_y);
        sin_theta = Fast_Sin(theta_y);

        // set the matrix up
        Mat_Init_4X4(&my, cos_theta, 0, -sin_theta, 0, 0, 1, 0, 0, sin_theta, 0,
                cos_theta, 0, 0, 0, 0, 1);

        // concatenate matrices
        Mat_Mul_4X4(&mx, &my, mrot);
        return;

    }
        break;

    case 4: // z rotation
    {
        // compute the sine and cosine of the angle
        cos_theta = Fast_Cos(theta_z);
        sin_theta = Fast_Sin(theta_z);

        // set the matrix up
        Mat_Init_4X4(&mz, cos_theta, sin_theta, 0, 0, -sin_theta, cos_theta, 0,
                0, 0, 0, 1, 0, 0, 0, 0, 1);

        // that's it, copy to output matrix
        MAT_COPY_4X4(&mz, mrot);
        return;

    }
        break;

    case 5: // xz rotation
    {
        // compute the sine and cosine of the angle x
        cos_theta = Fast_Cos(theta_x);
        sin_theta = Fast_Sin(theta_x);

        // set the matrix up
        Mat_Init_4X4(&mx, 1, 0, 0, 0, 0, cos_theta, sin_theta, 0, 0, -sin_theta,
                cos_theta, 0, 0, 0, 0, 1);

        // compute the sine and cosine of the angle z
        cos_theta = Fast_Cos(theta_z);
        sin_theta = Fast_Sin(theta_z);

        // set the matrix up
        Mat_Init_4X4(&mz, cos_theta, sin_theta, 0, 0, -sin_theta, cos_theta, 0,
                0, 0, 0, 1, 0, 0, 0, 0, 1);

        // concatenate matrices
        Mat_Mul_4X4(&mx, &mz, mrot);
        return;

    }
        break;

    case 6: // yz rotation
    {
        // compute the sine and cosine of the angle y
        cos_theta = Fast_Cos(theta_y);
        sin_theta = Fast_Sin(theta_y);

        // set the matrix up
        Mat_Init_4X4(&my, cos_theta, 0, -sin_theta, 0, 0, 1, 0, 0, sin_theta, 0,
                cos_theta, 0, 0, 0, 0, 1);

        // compute the sine and cosine of the angle z
        cos_theta = Fast_Cos(theta_z);
        sin_theta = Fast_Sin(theta_z);

        // set the matrix up
        Mat_Init_4X4(&mz, cos_theta, sin_theta, 0, 0, -sin_theta, cos_theta, 0,
                0, 0, 0, 1, 0, 0, 0, 0, 1);

        // concatenate matrices
        Mat_Mul_4X4(&my, &mz, mrot);
        return;

    }
        break;

    case 7: // xyz rotation
    {
        // compute the sine and cosine of the angle x
        cos_theta = Fast_Cos(theta_x);
        sin_theta = Fast_Sin(theta_x);

        // set the matrix up
        Mat_Init_4X4(&mx, 1, 0, 0, 0, 0, cos_theta, sin_theta, 0, 0, -sin_theta,
                cos_theta, 0, 0, 0, 0, 1);

        // compute the sine and cosine of the angle y
        cos_theta = Fast_Cos(theta_y);
        sin_theta = Fast_Sin(theta_y);

        // set the matrix up
        Mat_Init_4X4(&my, cos_theta, 0, -sin_theta, 0, 0, 1, 0, 0, sin_theta, 0,
                cos_theta, 0, 0, 0, 0, 1);

        // compute the sine and cosine of the angle z
        cos_theta = Fast_Cos(theta_z);
        sin_theta = Fast_Sin(theta_z);

        // set the matrix up
        Mat_Init_4X4(&mz, cos_theta, sin_theta, 0, 0, -sin_theta, cos_theta, 0,
                0, 0, 0, 1, 0, 0, 0, 0, 1);

        // concatenate matrices, watch order!
        Mat_Mul_4X4(&mx, &my, &mtmp);
        Mat_Mul_4X4(&mtmp, &mz, mrot);

    }
        break;

    default:
        break;

    } // end switch

} // end Build_XYZ_Rotation_MATRIX4X4     

/////////////////////////////////////////////////////////////
void Transform_To_World_OBJECT4DV1(OBJECT4DV1_PTR obj, // object to transform
        MATRIX4X4_PTR mt)   // transformation matrix
// should be transformed too
{
    // transform each local/model vertex of the object mesh and store result
    // in "transformed" vertex list
    for (int32_t vertex = 0; vertex < obj->num_vertices; vertex++) {
        // transform point from local to trans
        Mat_Mul_VECTOR4D_4X4(&obj->vlist_local[vertex], mt,
                &obj->vlist_trans[vertex]);

        // perform local/model to world transform
        VECTOR4D_Add(&obj->vlist_trans[vertex], &obj->world_pos,
                &obj->vlist_trans[vertex]);

    } // end for index
} // end Transform_OBJECT4DV1

////////////////////////////////////////////////////////////
void Transform_To_Screen_OBJECT4DV1(OBJECT4DV1_PTR obj, CAM4DV1_PTR cam) {
    float alpha = (0.5 * cam->viewport_width - 0.5);
    float beta = (0.5 * cam->viewport_height - 0.5);

    // Backfaces process each poly in mesh
    for (int32_t poly = 0; poly < obj->num_polys; poly++) {
        // acquire polygon
        POLY4DV1_PTR curr_poly = &obj->plist[poly];

        // extract vertex indices into master list, rember the polygons are
        // NOT self contained, but based on the vertex list stored in the object
        // itself
        int32_t vindex_0 = curr_poly->vert[0];
        int32_t vindex_1 = curr_poly->vert[1];
        int32_t vindex_2 = curr_poly->vert[2];

        // we need to compute the normal of this polygon face, and recall
        // that the vertices are in cw order, u = p0->p1, v=p0->p2, n=uxv
        VECTOR4D u, v, n;

        // build u, v
        VECTOR4D_Build(&obj->vlist_trans[vindex_0], &obj->vlist_trans[vindex_1],
                &u);
        VECTOR4D_Build(&obj->vlist_trans[vindex_0], &obj->vlist_trans[vindex_2],
                &v);

        // compute cross product
        VECTOR4D_Cross(&u, &v, &n);

        // now create eye vector to viewpoint
        VECTOR4D view;
        VECTOR4D_Build(&obj->vlist_trans[vindex_0], &cam->pos, &view);

        // and finally, compute the dot product
        float dp = VECTOR4D_Dot(&n, &view);

        // if the sign is > 0 then visible, 0 = scathing, < 0 invisible
        if (dp <= 0.0)
            SET_BIT_MATH3D(curr_poly->state, POLY4DV1_STATE_BACKFACE);

    } // end for poly

    // transfer obj world -> camera -> perspective -> screen
    for (int32_t vertex = 0; vertex < obj->num_vertices; vertex++) {
        POINT4D presult; // hold result of each transformation

        //world to camera
        Mat_Mul_VECTOR4D_4X4(&obj->vlist_trans[vertex], &cam->mcam, &presult);
        VECTOR4D_COPY(&obj->vlist_trans[vertex], &presult);

        //camera to perspective and screen
        float z = obj->vlist_trans[vertex].z;
        // transform the vertex by the view parameters in the camera
        obj->vlist_trans[vertex].x = cam->view_dist * obj->vlist_trans[vertex].x
                / z;
        obj->vlist_trans[vertex].y = cam->view_dist * obj->vlist_trans[vertex].y
                * cam->aspect_ratio / z;
        // z = z, so no change

        // transform the vertex by the view parameters in the camera
        obj->vlist_trans[vertex].x = alpha + alpha * obj->vlist_trans[vertex].x;
        obj->vlist_trans[vertex].y = beta - beta * obj->vlist_trans[vertex].y;

    } // end for vertex

} // end Remove_Backfaces_OBJECT4DV1

void Build_CAM4DV1_Matrix_Euler(CAM4DV1_PTR cam) {
    // this creates a camera matrix based on Euler angles
    // and stores it in the sent camera object
    // if you recall from chapter 6 to create the camera matrix
    // we need to create a transformation matrix that looks like:

    // Mcam = mt(-1) * my(-1) * mx(-1) * mz(-1)
    // that is the inverse of the camera translation matrix mutilplied
    // by the inverses of yxz, in that order, however, the order of
    // the rotation matrices is really up to you, so we aren't going
    // to force any order, thus its programmable based on the value
    // of cam_rot_seq which can be any value CAM_ROT_SEQ_XYZ where
    // XYZ can be in any order, YXZ, ZXY, etc.

    MATRIX4X4 mt_inv,  // inverse camera translation matrix
            mx_inv,  // inverse camera x axis rotation matrix
            my_inv,  // inverse camera y axis rotation matrix
            mz_inv,  // inverse camera z axis rotation matrix
            mrot,    // concatenated inverse rotation matrices
            mtmp;    // temporary working matrix

    // step 1: create the inverse translation matrix for the camera
    // position
    Mat_Init_4X4(&mt_inv, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -cam->pos.x,
            -cam->pos.y, -cam->pos.z, 1);

    // step 2: create the inverse rotation sequence for the camera
    // rember either the transpose of the normal rotation matrix or
    // plugging negative values into each of the rotations will result
    // in an inverse matrix

    // first compute all 3 rotation matrices

    // extract out euler angles
    float theta_x = cam->dir.x;
    float theta_y = cam->dir.y;
    float theta_z = cam->dir.z;

    // compute the sine and cosine of the angle x
    float cos_theta = Fast_Cos(theta_x);  // no change since cos(-x) = cos(x)
    float sin_theta = -Fast_Sin(theta_x); // sin(-x) = -sin(x)

    // set the matrix up
    Mat_Init_4X4(&mx_inv, 1, 0, 0, 0, 0, cos_theta, sin_theta, 0, 0, -sin_theta,
            cos_theta, 0, 0, 0, 0, 1);

    // compute the sine and cosine of the angle y
    cos_theta = Fast_Cos(theta_y);  // no change since cos(-x) = cos(x)
    sin_theta = -Fast_Sin(theta_y); // sin(-x) = -sin(x)

    // set the matrix up
    Mat_Init_4X4(&my_inv, cos_theta, 0, -sin_theta, 0, 0, 1, 0, 0, sin_theta, 0,
            cos_theta, 0, 0, 0, 0, 1);

    // compute the sine and cosine of the angle z
    cos_theta = Fast_Cos(theta_z);  // no change since cos(-x) = cos(x)
    sin_theta = -Fast_Sin(theta_z); // sin(-x) = -sin(x)

    // set the matrix up
    Mat_Init_4X4(&mz_inv, cos_theta, sin_theta, 0, 0, -sin_theta, cos_theta, 0,
            0, 0, 0, 1, 0, 0, 0, 0, 1);

    // now compute inverse camera rotation sequence ZYX
    Mat_Mul_4X4(&mz_inv, &my_inv, &mtmp);
    Mat_Mul_4X4(&mtmp, &mx_inv, &mrot);

    // now mrot holds the concatenated product of inverse rotation matrices
    // multiply the inverse translation matrix against it and store in the
    // camera objects' camera transform matrix we are done!
    Mat_Mul_4X4(&mt_inv, &mrot, &cam->mcam);

} // end Build_CAM4DV1_Matrix_Euler

/////////////////////////////////////////////////////////////
void Init_CAM4DV1(CAM4DV1_PTR cam,       // the camera object
        int32_t cam_attr,          // attributes
        POINT4D_PTR cam_pos,   // initial camera position
        VECTOR4D_PTR cam_dir,  // initial camera angles
        POINT4D_PTR cam_target, // UVN target
        float near_clip_z,     // near and far clipping planes
        float far_clip_z, float fov,             // field of view in degrees
        float viewport_width,  // size of final screen viewport
        float viewport_height) {
    // this function initializes the camera object cam, the function
    // doesn't do a lot of error checking or sanity checking since
    // I want to allow you to create projections as you wish, also
    // I tried to minimize the number of parameters the functions needs

    // first set up parms that are no brainers
    cam->attr = cam_attr;              // camera attributes

    VECTOR4D_COPY(&cam->pos, cam_pos); // positions
    VECTOR4D_COPY(&cam->dir, cam_dir); // direction vector or angles for
    // euler camera
//	// for UVN camera
//	VECTOR4D_INITXYZ(&cam->u, 1,0,0);  // set to +x
//	VECTOR4D_INITXYZ(&cam->v, 0,1,0);  // set to +y
//	VECTOR4D_INITXYZ(&cam->n, 0,0,1);  // set to +z        

//	if (cam_target!=NULL)
//		VECTOR4D_COPY(&cam->target, cam_target); // UVN target
//	else
//		VECTOR4D_ZERO(&cam->target);

    cam->near_clip_z = near_clip_z;     // near z=constant clipping plane
    cam->far_clip_z = far_clip_z;      // far z=constant clipping plane

    cam->viewport_width = viewport_width;   // dimensions of viewport
    cam->viewport_height = viewport_height;

    cam->viewport_center_x = (viewport_width - 1) / 2; // center of viewport
    cam->viewport_center_y = (viewport_height - 1) / 2;

    cam->aspect_ratio = (float) viewport_width / (float) viewport_height;

    // set all camera matrices to identity matrix
    MAT_IDENTITY_4X4(&cam->mcam);
    MAT_IDENTITY_4X4(&cam->mper);
    MAT_IDENTITY_4X4(&cam->mscr);

    // set independent vars
    cam->fov = fov;

    // set the viewplane dimensions up, they will be 2 x (2/ar)
    cam->viewplane_width = 2.0;
    cam->viewplane_height = 2.0 / cam->aspect_ratio;

    // now we know fov and we know the viewplane dimensions plug into formula and
    // solve for view distance parameters
    float tan_fov_div2 = tan(DEG_TO_RAD(fov/2));

    cam->view_dist = (0.5) * (cam->viewplane_width) * tan_fov_div2;

    // test for 90 fov first since it's easy :)
    if (fov == 90.0) {
        // set up the clipping planes -- easy for 90 degrees!
        POINT3D pt_origin; // point on the plane
        VECTOR3D_INITXYZ(&pt_origin, 0, 0, 0);

        VECTOR3D vn; // normal to plane

        // right clipping plane
        VECTOR3D_INITXYZ(&vn, 1, 0, -1); // x=z plane
        PLANE3D_Init(&cam->rt_clip_plane, &pt_origin, &vn, 1);

        // left clipping plane
        VECTOR3D_INITXYZ(&vn, -1, 0, -1); // -x=z plane
        PLANE3D_Init(&cam->lt_clip_plane, &pt_origin, &vn, 1);

        // top clipping plane
        VECTOR3D_INITXYZ(&vn, 0, 1, -1); // y=z plane
        PLANE3D_Init(&cam->tp_clip_plane, &pt_origin, &vn, 1);

        // bottom clipping plane
        VECTOR3D_INITXYZ(&vn, 0, -1, -1); // -y=z plane
        PLANE3D_Init(&cam->bt_clip_plane, &pt_origin, &vn, 1);
    } // end if d=1
    else {
        // now compute clipping planes yuck!
        POINT3D pt_origin; // point on the plane
        VECTOR3D_INITXYZ(&pt_origin, 0, 0, 0);

        VECTOR3D vn; // normal to plane

        // since we don't have a 90 fov, computing the normals
        // are a bit tricky, there are a number of geometric constructions
        // that solve the problem, but I'm going to solve for the
        // vectors that represent the 2D projections of the frustrum planes
        // on the x-z and y-z planes and then find perpendiculars to them

        // right clipping plane, check the math on graph paper
        VECTOR3D_INITXYZ(&vn, cam->view_dist, 0, -cam->viewplane_width / 2.0);
        PLANE3D_Init(&cam->rt_clip_plane, &pt_origin, &vn, 1);

        // left clipping plane, we can simply reflect the right normal about
        // the z axis since the planes are symetric about the z axis
        // thus invert x only
        VECTOR3D_INITXYZ(&vn, -cam->view_dist, 0, -cam->viewplane_width / 2.0);
        PLANE3D_Init(&cam->lt_clip_plane, &pt_origin, &vn, 1);

        // top clipping plane, same construction
        VECTOR3D_INITXYZ(&vn, 0, cam->view_dist, -cam->viewplane_width / 2.0);
        PLANE3D_Init(&cam->tp_clip_plane, &pt_origin, &vn, 1);

        // bottom clipping plane, same inversion
        VECTOR3D_INITXYZ(&vn, 0, -cam->view_dist, -cam->viewplane_width / 2.0);
        PLANE3D_Init(&cam->bt_clip_plane, &pt_origin, &vn, 1);
    } // end else

} // end Init_CAM4DV1

/////////////////////////////////////////////////////////////
void Adjust_Viewport_CAM4DV1(CAM4DV1_PTR cam,       // the camera object
        float viewport_width,  // size of final screen viewport
        float viewport_height) {
    cam->viewport_width = viewport_width;   // dimensions of viewport
    cam->viewport_height = viewport_height;

    cam->viewport_center_x = (viewport_width - 1) / 2; // center of viewport
    cam->viewport_center_y = (viewport_height - 1) / 2;

    cam->aspect_ratio = (float) viewport_width / (float) viewport_height;

    // set the viewplane dimensions up, they will be 2 x (2/ar)
    cam->viewplane_width = 2.0;
    cam->viewplane_height = 2.0 / cam->aspect_ratio;

    // now we know fov and we know the viewplane dimensions plug into formula and
    // solve for view distance parameters
    float tan_fov_div2 = tan(DEG_TO_RAD(cam->fov/2));

    cam->view_dist = (0.5) * (cam->viewplane_width) * tan_fov_div2;

} // end Adjust_Viewport_CAM4DV1

