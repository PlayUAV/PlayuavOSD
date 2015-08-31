#ifndef OSD_PROC_H__
#define OSD_PROC_H__

#include "board.h"

/// GPS status codes
enum GPS_Status {
	NO_GPS = 0,             ///< No GPS connected/detected
	NO_FIX = 1,             ///< Receiving valid GPS messages but no lock
	GPS_OK_FIX_2D = 2,      ///< Receiving valid messages and 2D lock
	GPS_OK_FIX_3D = 3,      ///< Receiving valid messages and 3D lock
	GPS_OK_FIX_3D_DGPS = 4, ///< Receiving valid messages and 3D lock with differential improvements
	GPS_OK_FIX_3D_RTK = 5,  ///< Receiving valid messages and 3D lock, with relative-positioning improvements
};
	
void vTaskOSD(void *pvParameters);

void RenderScreen(void);

void hud_draw_uav3d(void);
void hud_draw_uav2d(void);

void cal_vars(void);

void hud_draw_warnning(void);

void hud_draw_vertical_scale(int v, int range, int halign, int x, int y, int height, int mintick_step, 
							 int majtick_step, int mintick_len, int majtick_len,
							 int boundtick_len, __attribute__((unused)) int max_val, int flags);

void hud_draw_linear_compass(int v, int home_dir, int range, int width, int x, int y, int mintick_step, 
							 int majtick_step, int mintick_len, int majtick_len, 
							 __attribute__((unused)) int flags);

void draw_flight_mode(int x, int y, int xs, int ys, int va, int ha, int flags, int font);


void hud_draw_throttle(void);
void hud_draw_wind(void);
void hud_draw_CWH(void);							 
void hud_draw_head_wp_home(void);
void draw_map(void);
	
void DJI_test(void);							 

#endif //OSD_PROC_H__


