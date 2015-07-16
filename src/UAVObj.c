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
#include "m3dlib.h"
#include "UAVObj.h"
#include "osdcore.h"
#include "osdconfig.h"
#include "osdvar.h"

// initialize camera position and direction
POINT4D  cam_pos = {0,0,0,1};
VECTOR4D cam_dir = {0,0,0,1};

// all your initialization code goes here...
VECTOR4D vscale={5.0,5.0,5.0,1},  // scale of object
         vpos = {0,0,0,1},        // position of object
         vrot = {0,0,0,1};        // initial orientation of object

CAM4DV1    	cam;     // the single camera
OBJECT4DV1 	uav3D;  
		 
		 
POLYGON2D 	uav2D; 
POLYGON2D 	rollscale2D; 		 
		 
void cam3D_init(void)
{
	// initialize the camera with 90 FOV, normalized coordinates
	Init_CAM4DV1(&cam,      // the camera object
				 CAM_MODEL_EULER, // the euler model
				 &cam_pos,  // initial camera position
				 &cam_dir,  // initial camera angles
				 NULL,      // no target
				 50.0,      // near and far clipping planes
				 500.0,
				 90.0,      // field of view in degrees
				 GRAPHICS_RIGHT,   // size of final screen viewport
				 GRAPHICS_BOTTOM);
}
	
void uav3D_init(void)
{
	uint8_t i = 0;
	
	cam3D_init();
	
	uav3D.state = 3;
	uav3D.attr = 0;
	VECTOR4D_INITXYZ(&(uav3D.world_pos), 0.0, 0.0, 100.0);
	VECTOR4D_ZERO(&(uav3D.dir));

//////////////////////////////////////////////////////////
	uav3D.num_vertices = 8;

	VECTOR4D_INITXYZ(&(uav3D.vlist_local[0]), 0.0, 12.0, 0.0);
	VECTOR4D_INITXYZ(&(uav3D.vlist_local[1]), 5.0, -12.0, 0.0);
	VECTOR4D_INITXYZ(&(uav3D.vlist_local[2]), 0.0, -5.0, -5.0);
	VECTOR4D_INITXYZ(&(uav3D.vlist_local[3]), -5.0, -12.0, 0.0);
	VECTOR4D_INITXYZ(&(uav3D.vlist_local[4]), 10.0, -12.0, 0.0);
	VECTOR4D_INITXYZ(&(uav3D.vlist_local[5]), -10.0, -12.0, 0.0);
	VECTOR4D_INITXYZ(&(uav3D.vlist_local[6]), 2.0, 0.0, 0.0);
	VECTOR4D_INITXYZ(&(uav3D.vlist_local[7]), -2.0, 0.0, 0.0);

	for(i=0; i<uav3D.num_vertices; i++)
	{
		VECTOR4D_ZERO(&(uav3D.vlist_trans[i]));
	}

	
	uav3D.num_polys = 5;
	
	for(i=0; i<uav3D.num_polys; i++)
	{
		uav3D.plist[i].state = 1;
		uav3D.plist[i].attr = 137;
		uav3D.plist[i].color = 1920;
		VECTOR4D_COPY(uav3D.plist[i].vlist, uav3D.vlist_local);
	}
	
	uav3D.plist[0].vert[0] = 0;
	uav3D.plist[0].vert[1] = 1;
	uav3D.plist[0].vert[2] = 2;

	uav3D.plist[1].vert[0] = 6;
	uav3D.plist[1].vert[1] = 4;
	uav3D.plist[1].vert[2] = 1;
	
	uav3D.plist[2].vert[0] = 0;
	uav3D.plist[2].vert[1] = 2;
	uav3D.plist[2].vert[2] = 3;
	
	uav3D.plist[3].vert[0] = 7;
	uav3D.plist[3].vert[1] = 3;
	uav3D.plist[3].vert[2] = 5;
	
	uav3D.plist[4].vert[0] = 2;
	uav3D.plist[4].vert[1] = 1;
	uav3D.plist[4].vert[2] = 3;


    VECTOR4D v;
    //translate the obj
    VECTOR4D_INITXYZ(&v, eeprom_buffer.params.Atti_3D_posX - GRAPHICS_X_MIDDLE,
                         -eeprom_buffer.params.Atti_3D_posY + GRAPHICS_Y_MIDDLE, 0);
    Translate_OBJECT4DV1(&uav3D, &v);
    //scale the mode
    VECTOR4D_INITXYZ(&v, atti_3d_scale, atti_3d_scale, 0);
    Scale_OBJECT4DV1(&uav3D, &v);
	
}

	
void uav2D_init(void)
{
	// initialize uav2d
	uav2D.state       = 1;   // turn it on
	uav2D.num_verts   = 38;  
	uav2D.x0          = eeprom_buffer.params.Atti_mp_posX; // position it
	uav2D.y0          = eeprom_buffer.params.Atti_mp_posY;

	int index=0, i=0;
	const int lX = 5;
	const int stepY = 11;
	const int hX = 22;
	for(index=0; index<uav2D.num_verts/2-1;)
	{
        VECTOR2D_INITXYZ(&(uav2D.vlist_local[index]), -lX, stepY*(i+1));
        VECTOR2D_INITXYZ(&(uav2D.vlist_local[index+1]), lX, stepY*(i+1));
		index += 2;
		i++;
	}
	VECTOR2D_INITXYZ(&(uav2D.vlist_local[index]), -hX, 0);
	VECTOR2D_INITXYZ(&(uav2D.vlist_local[index+1]), hX, 0);
	index += 2;
	
	i=0;
	for(;index<uav2D.num_verts;)
	{
        VECTOR2D_INITXYZ(&(uav2D.vlist_local[index]), -lX, -stepY*(i+1));
        VECTOR2D_INITXYZ(&(uav2D.vlist_local[index+1]), lX, -stepY*(i+1));
		index += 2;
		i++;
	}


	// do a quick scale on the vertices
	Scale_Polygon2D(&uav2D, atti_mp_scale, atti_mp_scale);

	// initialize roll scale
	rollscale2D.state       = 1;   // turn it on
	rollscale2D.num_verts   = 13;  
	rollscale2D.x0          = eeprom_buffer.params.Atti_mp_posX; // position it
	rollscale2D.y0          = eeprom_buffer.params.Atti_mp_posY;
	
	
	int x, y, theta;
	int mp = (rollscale2D.num_verts-1)/2;
	i=mp;
	int radio = 38;
	int arcStep = (mp*10)/6;
	for(index=0; index<mp;index++)
	{
		theta = i*arcStep;
		x = radio* Fast_Sin(theta);
		y = radio * Fast_Cos(theta);
		VECTOR2D_INITXYZ(&(rollscale2D.vlist_local[index]), -x, -y);
		VECTOR2D_INITXYZ(&(rollscale2D.vlist_local[rollscale2D.num_verts-1-index]), x, -y);
		i--;
	}
	
	VECTOR2D_INITXYZ(&(rollscale2D.vlist_local[index]), 0, -radio);
    Scale_Polygon2D(&rollscale2D, atti_mp_scale, atti_mp_scale);
}


