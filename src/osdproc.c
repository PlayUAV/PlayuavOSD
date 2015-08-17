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
 * With Grateful Acknowledgements to the projects:
 * Tau Labs - Brain FPV Flight Controller(https://github.com/BrainFPV/TauLabs)
 */
#include "osdproc.h"
#include "graphengine.h"
#include "led.h"
#include "osdcore.h"
#include "osdvar.h"
#include "fonts.h"
#include "UAVObj.h"
#include "osdconfig.h"

#define HUD_VSCALE_FLAG_CLEAR       1
#define HUD_VSCALE_FLAG_NO_NEGATIVE 2

extern xSemaphoreHandle onScreenDisplaySemaphore;

int32_t test_alt, test_speed, test_throttle;

//2:small, 0:normal, 3:large
const int SIZE_TO_FONT[3] = {2, 0, 3};

uint8_t last_panel = 1;
int32_t new_panel_start_time = 0;

uint8_t last_warn_type = 0;
int32_t last_warn_time = 0;
char* warn_str = "";

const char METRIC_SPEED[] = "KM/H";         //kilometer per hour
const char METRIC_DIST_SHORT[] = "M";       //meter
const char METRIC_DIST_LONG[] = "KM";       //kilometer

const char IMPERIAL_SPEED[] = "M/H";        //mile per hour
const char IMPERIAL_DIST_SHORT[] = "F";     //feet
const char IMPERIAL_DIST_LONG[] = "M";      //mile

// Unit conversion constants
float convert_speed = 0.0f;
float convert_distance = 0.0f;
float convert_distance_divider = 0.0f;
const char * dist_unit_short = METRIC_DIST_SHORT;
const char * dist_unit_long = METRIC_DIST_LONG;
const char * spd_unit = METRIC_SPEED;

void do_converts(void)
{
	if (eeprom_buffer.params.Units_mode == 1)
	{
		convert_distance = 2.23f;
		convert_speed = 3.28f;
		convert_distance_divider = 5280.0f; // feet in a mile
		dist_unit_short = IMPERIAL_DIST_SHORT;
		dist_unit_long = IMPERIAL_DIST_LONG;
		spd_unit = IMPERIAL_SPEED;
	}
	else
	{
		convert_distance = 1.0f;
		convert_speed = 3.6f;
		convert_distance_divider = 1000.0f;
		dist_unit_short = METRIC_DIST_SHORT;
		dist_unit_long = METRIC_DIST_LONG;
		spd_unit = METRIC_SPEED;
	}
}

bool bShownAtPanle(uint16_t itemPanel)
{
	//issue #1 - fixed
	return ((itemPanel & (1<<(current_panel-1))) != 0);
}

void vTaskOSD(void *pvParameters)
{	
    uav3D_init();
    uav2D_init();

	osdCoreInit();
	osdVideoSetXOffset(eeprom_buffer.params.osd_offsetX);
	osdVideoSetYOffset(eeprom_buffer.params.osd_offsetY);
	
	for(;;)
	{
		xSemaphoreTake(onScreenDisplaySemaphore, portMAX_DELAY);
		
		clearGraphics();

		RenderScreen();
	}
}

void RenderScreen(void)
{
    char tmp_str[50] = { 0 };
    char* tmp_str1 = "";
    int16_t tmp_int16;
    int tmp_int1, tmp_int2;
    uint32_t time_now;

    do_converts();

//  DJI_test();
//  return;
    if(current_panel > eeprom_buffer.params.Max_panels)
        current_panel = 1;

    // mode
    if (eeprom_buffer.params.FlightMode_en==1 && bShownAtPanle(eeprom_buffer.params.FlightMode_panel)) {
        draw_flight_mode(eeprom_buffer.params.FlightMode_posX,
                         eeprom_buffer.params.FlightMode_posY, 0, 0, TEXT_VA_TOP,
                         eeprom_buffer.params.FlightMode_align, 0,
                         SIZE_TO_FONT[eeprom_buffer.params.FlightMode_fontsize]);
    }

    // arming status
    if (eeprom_buffer.params.Arm_en==1 && bShownAtPanle(eeprom_buffer.params.Arm_panel)) {
        tmp_str1 = motor_armed ? "ARMED" : "DISARMED";
        write_string(tmp_str1, eeprom_buffer.params.Arm_posX,
                     eeprom_buffer.params.Arm_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.Arm_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.Arm_fontsize]);
    }

    //Battery
    if (eeprom_buffer.params.BattVolt_en==1 && bShownAtPanle(eeprom_buffer.params.BattVolt_panel)) {
        sprintf(tmp_str, "%0.1fV", (double) osd_vbat_A);
        write_string(tmp_str, eeprom_buffer.params.BattVolt_posX,
                     eeprom_buffer.params.BattVolt_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.BattVolt_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.BattVolt_fontsize]);
    }
    if (eeprom_buffer.params.BattCurrent_en==1 && bShownAtPanle(eeprom_buffer.params.BattCurrent_panel)) {
        sprintf(tmp_str, "%0.1fA", (double) (osd_curr_A * 0.01));
        write_string(tmp_str, eeprom_buffer.params.BattCurrent_posX,
                     eeprom_buffer.params.BattCurrent_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.BattCurrent_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.BattCurrent_fontsize]);
    }
    if (eeprom_buffer.params.BattRemaining_en==1 && bShownAtPanle(eeprom_buffer.params.BattRemaining_panel)) {
        sprintf(tmp_str, "%d/", osd_battery_remaining_A);
        write_string(tmp_str, eeprom_buffer.params.BattRemaining_posX,
                     eeprom_buffer.params.BattRemaining_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.BattRemaining_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.BattRemaining_fontsize]);
    }
    if (eeprom_buffer.params.BattConsumed_en==1 && bShownAtPanle(eeprom_buffer.params.BattConsumed_panel)) {
        sprintf(tmp_str, "%dmah", (int)osd_curr_consumed_mah);
        write_string(tmp_str, eeprom_buffer.params.BattConsumed_posX,
                     eeprom_buffer.params.BattConsumed_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.BattConsumed_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.BattConsumed_fontsize]);
    }

    //altitude
    if (eeprom_buffer.params.Alt_Scale_en==1 && bShownAtPanle(eeprom_buffer.params.Alt_Scale_panle)) {
        hud_draw_vertical_scale(osd_alt * convert_distance, 60,
                eeprom_buffer.params.Alt_Scale_align,
                eeprom_buffer.params.Alt_Scale_posX,
                eeprom_buffer.params.Alt_Scale_posY, 72, 10, 20, 5, 8, 11,
                10000, 0);
    }

    if (eeprom_buffer.params.TALT_en==1 && bShownAtPanle(eeprom_buffer.params.TALT_panel)) {
        float tmp = osd_alt * convert_distance;
        if (tmp < convert_distance_divider){
            sprintf(tmp_str, "ALT: %d%s", (int) tmp, dist_unit_short);
        }
        else{
            sprintf(tmp_str, "ALT: %0.2f%s", (double) (tmp / convert_distance_divider), dist_unit_long);
        }

        //sprintf(tmp_str, "ALT:%i", (int)osd_alt);
        write_string(tmp_str, eeprom_buffer.params.TALT_posX,
                     eeprom_buffer.params.TALT_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.TALT_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.TALT_fontsize]);
    }

    //speed
    if (eeprom_buffer.params.Speed_scale_en==1 && bShownAtPanle(eeprom_buffer.params.Speed_scale_panel)) {
        hud_draw_vertical_scale(osd_groundspeed * convert_speed, 60,
                eeprom_buffer.params.Speed_scale_align,
                eeprom_buffer.params.Speed_scale_posX,
                eeprom_buffer.params.Speed_scale_posY, 72, 10, 20, 5, 8, 11,
                100, 0);
    }

    if (eeprom_buffer.params.TSPD_en==1 && bShownAtPanle(eeprom_buffer.params.TSPD_panel)) {
        float tmp = osd_groundspeed * convert_speed;
        sprintf(tmp_str, "SPD:%d%s", (int) tmp, spd_unit);
        write_string(tmp_str, eeprom_buffer.params.TSPD_posX,
                     eeprom_buffer.params.TSPD_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.TSPD_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.TSPD_fontsize]);
    }

    //uav attitude
    if(eeprom_buffer.params.Atti_3D_en==1 && bShownAtPanle(eeprom_buffer.params.Atti_3D_panel)){
        hud_draw_uav3d();
    }

    if(eeprom_buffer.params.Atti_mp_en==1 && bShownAtPanle(eeprom_buffer.params.Atti_mp_panel)){
        hud_draw_uav2d();
    }

    // throttle with percent
    if(eeprom_buffer.params.Throt_en==1 && bShownAtPanle(eeprom_buffer.params.Throt_panel)){
        hud_draw_throttle();
    }

    //GPS
    if (eeprom_buffer.params.GpsStatus_en==1 && bShownAtPanle(eeprom_buffer.params.GpsStatus_panel)) {
        switch (osd_fix_type) {
        case NO_GPS:
        case NO_FIX:
            sprintf(tmp_str, "NOFIX");
            break;
        case GPS_OK_FIX_2D:
            sprintf(tmp_str, "FIX2D-%d", (int) osd_satellites_visible);
            break;
        case GPS_OK_FIX_3D:
            sprintf(tmp_str, "FIX3D-%d", (int) osd_satellites_visible);
            break;
        case GPS_OK_FIX_3D_DGPS:
            sprintf(tmp_str, "FIXD3D-%d", (int) osd_satellites_visible);
            break;
        default:
            sprintf(tmp_str, "NOGPS");
            break;
        }
        write_string(tmp_str, eeprom_buffer.params.GpsStatus_posX,
                     eeprom_buffer.params.GpsStatus_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.GpsStatus_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.GpsStatus_fontsize]);
    }

    if (eeprom_buffer.params.GpsHDOP_en==1 && bShownAtPanle(eeprom_buffer.params.GpsHDOP_panel)) {
        sprintf(tmp_str, "HDOP:%0.2f", (double) osd_hdop / 100.0f);
        write_string(tmp_str, eeprom_buffer.params.GpsHDOP_posX,
                     eeprom_buffer.params.GpsHDOP_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.GpsHDOP_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.GpsHDOP_fontsize]);
    }

    if (eeprom_buffer.params.GpsLat_en==1 && bShownAtPanle(eeprom_buffer.params.GpsLat_panle)) {
        sprintf(tmp_str, "%0.5f", (double) osd_lat / 10000000.0f);
        write_string(tmp_str, eeprom_buffer.params.GpsLat_posX,
                     eeprom_buffer.params.GpsLat_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.GpsLat_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.GpsLat_fontsize]);
    }
    if (eeprom_buffer.params.GpsLon_en==1 && bShownAtPanle(eeprom_buffer.params.GpsLon_panel)) {
        sprintf(tmp_str, "%0.5f", (double) osd_lon / 10000000.0f);
        write_string(tmp_str, eeprom_buffer.params.GpsLon_posX,
                     eeprom_buffer.params.GpsLon_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.GpsLon_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.GpsLon_fontsize]);
    }

    //GPS2
    if (eeprom_buffer.params.Gps2Status_en==1 && bShownAtPanle(eeprom_buffer.params.Gps2Status_panel)) {
        switch (osd_fix_type2) {
        case NO_GPS:
        case NO_FIX:
            sprintf(tmp_str, "NOFIX");
            break;
        case GPS_OK_FIX_2D:
            sprintf(tmp_str, "FIX2D-%d", (int) osd_satellites_visible2);
            break;
        case GPS_OK_FIX_3D:
            sprintf(tmp_str, "FIX3D-%d", (int) osd_satellites_visible2);
            break;
        case GPS_OK_FIX_3D_DGPS:
            sprintf(tmp_str, "FIXD3D-%d", (int) osd_satellites_visible2);
            break;
        default:
            sprintf(tmp_str, "NOGPS");
            break;
        }
        write_string(tmp_str, eeprom_buffer.params.Gps2Status_posX,
                     eeprom_buffer.params.Gps2Status_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.Gps2Status_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.Gps2Status_fontsize]);
    }
    if (eeprom_buffer.params.Gps2HDOP_en==1 && bShownAtPanle(eeprom_buffer.params.Gps2HDOP_panel)) {
        sprintf(tmp_str, "HDOP:%0.2f", (double) osd_hdop2 / 100.0f);
        write_string(tmp_str, eeprom_buffer.params.Gps2HDOP_posX,
                     eeprom_buffer.params.Gps2HDOP_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.Gps2HDOP_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.Gps2HDOP_fontsize]);
    }
    if (eeprom_buffer.params.Gps2Lat_en==1 && bShownAtPanle(eeprom_buffer.params.Gps2Lat_panel)) {
        sprintf(tmp_str, "%0.5f", (double) osd_lat2 / 10000000.0f);
        write_string(tmp_str, eeprom_buffer.params.Gps2Lat_posX,
                     eeprom_buffer.params.Gps2Lat_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.Gps2Lat_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.Gps2Lat_fontsize]);
    }
    if (eeprom_buffer.params.Gps2Lon_en==1 && bShownAtPanle(eeprom_buffer.params.Gps2Lon_panel)) {
        sprintf(tmp_str, "%0.5f", (double) osd_lon2 / 10000000.0f);
        write_string(tmp_str, eeprom_buffer.params.Gps2Lon_posX,
                     eeprom_buffer.params.Gps2Lon_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.Gps2Lon_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.Gps2Lon_fontsize]);
    }

    // total trip distance since startup
    if (eeprom_buffer.params.TotalTripDist_en==1 && bShownAtPanle(eeprom_buffer.params.TotalTripDist_panel)) {
        float tmp = osd_total_trip_dist * convert_distance;
        if (tmp < convert_distance_divider){
            sprintf(tmp_str, "%d%s", (int) tmp, dist_unit_short);
        }
        else{
            sprintf(tmp_str, "%0.2f%s", (double) (tmp / convert_distance_divider), dist_unit_long);
        }
        write_string(tmp_str, eeprom_buffer.params.TotalTripDist_posX,
                     eeprom_buffer.params.TotalTripDist_posY, 0, 0, TEXT_VA_TOP,
                     eeprom_buffer.params.TotalTripDist_align, 0,
                     SIZE_TO_FONT[eeprom_buffer.params.TotalTripDist_fontsize]);
    }

    // time
    if (!last_motor_armed && motor_armed) {
        armed_start_time = GetSystimeMS();
    }

    if (!motor_armed){
        armed_start_time = 0;
    }

    if (eeprom_buffer.params.Time_en==1 && bShownAtPanle(eeprom_buffer.params.Time_panel)) {
        time_now = GetSystimeMS() - sys_start_time;

        if (eeprom_buffer.params.Time_type == 1) {
            time_now = heatbeat_start_time ? (GetSystimeMS() - heatbeat_start_time) : 0;
        } else if (eeprom_buffer.params.Time_type == 2) {
            time_now = armed_start_time ? (GetSystimeMS() - armed_start_time) : 0;
        }

        tmp_int16 = (time_now / 3600000); // hours
        if (tmp_int16 == 0) {
            tmp_int1 = time_now / 60000; // minutes
            tmp_int2 = (time_now / 1000) - 60 * tmp_int1; // seconds
            sprintf(tmp_str, "%02d:%02d", (int) tmp_int1, (int) tmp_int2);
        } else {
            tmp_int1 = time_now / 60000 - 60 * tmp_int16; // minutes
            tmp_int2 = (time_now / 1000) - 60 * tmp_int1 - 3600 * tmp_int16; // seconds
            sprintf(tmp_str, "%02d:%02d:%02d", (int) tmp_int16, (int) tmp_int1, (int) tmp_int2);
        }
        write_string(tmp_str, eeprom_buffer.params.Time_posX,
                eeprom_buffer.params.Time_posY, 0, 0, TEXT_VA_TOP,
                eeprom_buffer.params.Time_align, 0,
                SIZE_TO_FONT[eeprom_buffer.params.Time_fontsize]);
    }

    //draw Compass, homedir, homedist, waypointdir, waypointdist
    hud_draw_CWH();

    int x = 5;
    int y = 220;
    //Vertical Speed(climb rate) in m/s
    if (eeprom_buffer.params.ClimbRate_en==1 && bShownAtPanle(eeprom_buffer.params.ClimbRate_panel)) {
        int arrlen = 6;
        x = eeprom_buffer.params.ClimbRate_posX;
        y = eeprom_buffer.params.ClimbRate_posY;
        sprintf(tmp_str, "%0.2f", (double) fabs(osd_climb));
        write_string(tmp_str, x + 5, y, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_LEFT, 0,
                SIZE_TO_FONT[eeprom_buffer.params.ClimbRate_fontsize]);
        if (eeprom_buffer.params.ClimbRate_fontsize != 0)
            arrlen += 2;

        if (osd_climb > 0.0f) //ascent
                {
            write_vline_lm(x, y - arrlen, y + arrlen, 1, 1);
            write_line_outlined(x - 3, y - arrlen + 3, x, y - arrlen, 2, 2, 0, 1);
            write_line_outlined(x + 3, y - arrlen + 3, x, y - arrlen, 2, 2, 0, 1);
        } else if (osd_climb < 0.0f) //descent
                {
            write_vline_lm(x, y - arrlen, y + arrlen, 1, 1);
            write_line_outlined(x - 3, y + arrlen - 3, x, y + arrlen, 2, 2, 0, 1);
            write_line_outlined(x + 3, y + arrlen - 3, x, y + arrlen, 2, 2, 0, 1);
        }
    }

    //RSSI
    if (eeprom_buffer.params.RSSI_en==1 && bShownAtPanle(eeprom_buffer.params.RSSI_panel)) {
        int rssi = osd_rssi;
        x = eeprom_buffer.params.RSSI_posX;
        y = eeprom_buffer.params.RSSI_posY;

        //0:percentage 1:raw
        if (eeprom_buffer.params.RSSI_raw_en == 0) {
            uint16_t rssiMin = eeprom_buffer.params.RSSI_min;
            uint16_t rssiMax = eeprom_buffer.params.RSSI_max;
            if (rssiMin < 0)
                rssiMin = 0;
            if (rssiMax > 255)
                rssiMax = 255;

            if ((rssiMax - rssiMin) > 0)
                rssi = (int) ((float) (rssi - rssiMin) / (float) (rssiMax - rssiMin) * 100.0f);

            if (rssi < 0)
                rssi = 0;
        }
        sprintf(tmp_str, "RSSI:%d/", rssi);
        write_string(tmp_str, x, y, 0, 0, TEXT_VA_MIDDLE,
                eeprom_buffer.params.RSSI_align, 0,
                SIZE_TO_FONT[eeprom_buffer.params.RSSI_fontsize]);
    }

    //tip which panel are the current panel for 3 secs
    if (last_panel != current_panel) {
        last_panel = current_panel;
        new_panel_start_time = GetSystimeMS();
    }
    if ((GetSystimeMS() - new_panel_start_time) < 3000) {
        sprintf(tmp_str, "P:%d", (int) current_panel);
        write_string(tmp_str, GRAPHICS_X_MIDDLE, 210, 0, 0, TEXT_VA_TOP,
                TEXT_HA_CENTER, 0, SIZE_TO_FONT[1]);
    }

    if (eeprom_buffer.params.Wind_en==1 && bShownAtPanle(eeprom_buffer.params.Wind_panel)) {
        hud_draw_wind();
    }

    //warnning - should be displayed lastly in case not be covered by others
    hud_draw_warnning();
}

void draw_flight_mode(int x, int y, int xs, int ys, int va, int ha, int flags, int font)
{
    char* mode_str = "unknown";
    if (apm_mav_type != 1){ //ArduCopter MultiRotor or ArduCopter Heli
        if (osd_mode == 0)       mode_str = "STAB"; //Stabilize: hold level position
        else if (osd_mode == 1)  mode_str = "ACRO"; //Acrobatic: rate control
        else if (osd_mode == 2)  mode_str = "ALTH"; //Altitude Hold: auto control
        else if (osd_mode == 3)  mode_str = "AUTO"; //Auto: auto control
        else if (osd_mode == 4)  mode_str = "GUID"; //Guided: auto control
        else if (osd_mode == 5)  mode_str = "LOIT"; //Loiter: hold a single location
        else if (osd_mode == 6)  mode_str = "RETL"; //Return to Launch: auto control
        else if (osd_mode == 7)  mode_str = "CIRC"; //Circle: auto control
        //else if (osd_mode == 8)  mode_str = "POSI"; //Position: auto control
        else if (osd_mode == 9)  mode_str = "LAND"; //Land:: auto control
        else if (osd_mode == 10) mode_str = "OFLO"; //OF_Loiter: hold a single location using optical flow sensor
        else if (osd_mode == 11) mode_str = "DRIF"; //Drift mode: 
        else if (osd_mode == 13) mode_str = "SPRT"; //Sport: earth frame rate control
        else if (osd_mode == 14) mode_str = "FLIP"; //Flip: flip the vehicle on the roll axis
        else if (osd_mode == 15) mode_str = "ATUN"; //Auto Tune: autotune the vehicle's roll and pitch gains
        else if (osd_mode == 16) mode_str = "POSH"; //Hybrid: position hold with manual override
    } else if(apm_mav_type == 1){ //ArduPlane
        if (osd_mode == 0)       mode_str = "MANU"; //Manual
        else if (osd_mode == 1)  mode_str = "CIRC"; //Circle
        else if (osd_mode == 2)  mode_str = "STAB"; //Stabilize
        else if (osd_mode == 3)  mode_str = "TRNG"; //Training
        else if (osd_mode == 4)  mode_str = "ACRO"; //Acro
        else if (osd_mode == 5)  mode_str = "FBWA"; //Fly_By_Wire_A
        else if (osd_mode == 6)  mode_str = "FBWB"; //Fly_By_Wire_B
        else if (osd_mode == 7)  mode_str = "CRUI"; //Cruise
        else if (osd_mode == 8)  mode_str = "ATUN"; //Auto Tune
        else if (osd_mode == 10) mode_str = "AUTO"; //Auto
        else if (osd_mode == 11) mode_str = "RETL"; //Return to Launch
        else if (osd_mode == 12) mode_str = "LOIT"; //Loiter
        else if (osd_mode == 15) mode_str = "GUID"; //Guided
        else if (osd_mode == 16) mode_str = "INIT"; //Initializing
    }

    write_string(mode_str, x, y, xs, ys, va, ha, flags, font);
}

void hud_draw_uav3d(void)
{
    static MATRIX4X4 mrot; // general rotation matrix

    static int32_t roll = 0;
    static int32_t pitch=0;
    static int32_t yaw=0;
    int x = eeprom_buffer.params.Atti_3D_posX;
    int y = eeprom_buffer.params.Atti_3D_posY;
    int map_radius = (int)(eeprom_buffer.params.Atti_3D_map_radius * atti_3d_scale);

    write_string("N", x, y-map_radius, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, 0, SIZE_TO_FONT[0]);
    write_string("E", x+map_radius, y, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, SIZE_TO_FONT[0]);
    //need to adjust viewport base on the video mode
    Adjust_Viewport_CAM4DV1(&cam, GRAPHICS_RIGHT, GRAPHICS_BOTTOM);

    roll = ((int32_t)osd_roll + 360) %360;
    roll = fabs(roll-360);

    pitch = ((int32_t)osd_pitch + 360) %360;
    pitch = fabs(pitch-360);

    yaw = fabs(osd_heading-360);

    Reset_OBJECT4DV1(&uav3D);

    // generate rotation matrix around
    Build_XYZ_Rotation_MATRIX4X4(pitch, roll, yaw, &mrot);

    // rotate and transfer to world coord
    Transform_To_World_OBJECT4DV1(&uav3D, &mrot);

    // generate camera matrix
    Build_CAM4DV1_Matrix_Euler(&cam);

    // transfer obj world -> camera -> perspective -> screen
    // then perform backfaces remove
    Transform_To_Screen_OBJECT4DV1(&uav3D, &cam);

    //draw object wire
    for (int poly=0; poly < uav3D.num_polys; poly++)
    {
        // render this polygon if and only if it's not clipped, not culled,
        // active, and visible, note however the concecpt of "backface" is
        // irrelevant in a wire frame engine though
        if (!(uav3D.plist[poly].state & POLY4DV1_STATE_ACTIVE) ||
             (uav3D.plist[poly].state & POLY4DV1_STATE_CLIPPED ) ||
             (uav3D.plist[poly].state & POLY4DV1_STATE_BACKFACE) )
           continue; // move onto next poly

        // extract vertex indices into master list, rember the polygons are
        // NOT self contained, but based on the vertex list stored in the object
        // itself
        int vindex_0 = uav3D.plist[poly].vert[0];
        int vindex_1 = uav3D.plist[poly].vert[1];
        int vindex_2 = uav3D.plist[poly].vert[2];

        // draw the lines now
        write_line_outlined(uav3D.vlist_trans[ vindex_0 ].x, uav3D.vlist_trans[ vindex_0 ].y,
                            uav3D.vlist_trans[ vindex_1 ].x, uav3D.vlist_trans[ vindex_1 ].y,
                            2, 2, 0, 1);

        write_line_outlined(uav3D.vlist_trans[ vindex_1 ].x, uav3D.vlist_trans[ vindex_1 ].y,
                            uav3D.vlist_trans[ vindex_2 ].x, uav3D.vlist_trans[ vindex_2 ].y,
                            2, 2, 0, 1);

        write_line_outlined(uav3D.vlist_trans[ vindex_2 ].x, uav3D.vlist_trans[ vindex_2 ].y,
                            uav3D.vlist_trans[ vindex_0 ].x, uav3D.vlist_trans[ vindex_0 ].y,
                            2, 2, 0, 1);

    } // end for poly
}

void hud_draw_uav2d()
{
    int index = 0;

    Reset_Polygon2D(&uav2D);
    Transform_Polygon2D(&uav2D, -osd_roll, 0, osd_pitch);

    // loop thru and draw a line from vertices 1 to n
    VECTOR4D v;
    for (index=0; index < uav2D.num_verts-1; )
    {
        VECTOR4D_INITXYZW(&v,   uav2D.vlist_trans[index].x+uav2D.x0, uav2D.vlist_trans[index].y+uav2D.y0,
                                uav2D.vlist_trans[index+1].x+uav2D.x0, uav2D.vlist_trans[index+1].y+uav2D.y0);
        if(Clip_Line(&v))
        {
            write_line_outlined(v.x, v.y, v.z, v.w, 2, 2, 0, 1);
        }
        index += 2;
    } // end for

    //rotate roll scale and display, we only cal x
    Reset_Polygon2D(&rollscale2D);
    Rotate_Polygon2D(&rollscale2D, -osd_roll);
    for (index=0; index < rollscale2D.num_verts-1; index++)
    {
        // draw line from ith to ith+1 vertex
        write_line_outlined(rollscale2D.vlist_trans[index].x + rollscale2D.x0, rollscale2D.vlist_trans[index].y + rollscale2D.y0,
                            rollscale2D.vlist_trans[index+1].x + rollscale2D.x0, rollscale2D.vlist_trans[index+1].y + rollscale2D.y0,
                            2, 2, 0, 1);
    } // end for

    int x = eeprom_buffer.params.Atti_mp_posX;
    int y = eeprom_buffer.params.Atti_mp_posY;
    int wingStart = (int)(12.0f * atti_mp_scale);
    int wingEnd = (int)(7.0f * atti_mp_scale);
    char tmp_str[10] = {0};
    //draw uav
    write_line_outlined(x, y,x-9,y+5, 2, 2, 0, 1);
    write_line_outlined(x, y,x+9,y+5, 2, 2, 0, 1);
    write_line_outlined(x-wingStart, y,x-wingEnd,y, 2, 2, 0, 1);
    write_line_outlined(x+wingEnd, y,x+wingStart,y, 2, 2, 0, 1);


    write_filled_rectangle_lm(x-9, y+6, 15, 9, 0, 1);
    sprintf(tmp_str, "%d", (int)osd_pitch);
    write_string(tmp_str, x, y+5, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, 0, SIZE_TO_FONT[0]);

    y = eeprom_buffer.params.Atti_mp_posY-(int)(38.0f * atti_mp_scale);
    //draw roll value
    write_line_outlined(x, y, x-4, y+8, 2, 2, 0, 1);
    write_line_outlined(x, y, x+4, y+8, 2, 2, 0, 1);
    write_line_outlined(x-4, y+8, x+4, y+8, 2, 2, 0, 1);
    sprintf(tmp_str, "%d", (int)osd_roll);
    write_string(tmp_str, x, y-3, 0, 0, TEXT_VA_BOTTOM, TEXT_HA_CENTER, 0, SIZE_TO_FONT[0]);
}

void hud_draw_throttle(void)
{
    char tmp_str[10] = { 0 };
    int16_t pos_th_y, pos_th_x;
    int posX, posY;
    posX = eeprom_buffer.params.Throt_posX;
    posY = eeprom_buffer.params.Throt_posY;

    pos_th_y = (int16_t)(0.5*osd_throttle);
    pos_th_x = posX - 25 + pos_th_y;
    sprintf(tmp_str, "%d", (int32_t)osd_throttle);
    write_string(tmp_str, posX, posY, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[0]);

//  if(eeprom_buffer.params.Throt_scale_en){
//      write_filled_rectangle_lm(posX-25, posY+10, pos_th_x, 5, 1, 1);
//      write_hline_lm(pos_th_x, posX+25, posY+10, 1, 1);
//      write_hline_lm(pos_th_x, posX+25, posY+14, 1, 1);
//      write_vline_lm(posX+25, posY+10, posY+14, 1, 1);
//      write_vline_lm(posX-25, posY+10, posY+14, 1, 1);
//  }

    if(eeprom_buffer.params.Throt_scale_en){
        if(eeprom_buffer.params.Throttle_Scale_Type == 0){
            write_filled_rectangle_lm(posX+3, posY+25-pos_th_y, 5, pos_th_y, 1, 1);
            write_hline_lm(posX+3, posX+7, posY-25, 1, 1);
            write_hline_lm(posX+3, posX+7, posY+25-pos_th_y, 1, 1);
            write_vline_lm(posX+3, posY-25, posY+25-pos_th_y, 1, 1);
            write_vline_lm(posX+7, posY-25, posY+25-pos_th_y, 1, 1);
        }
        else if(eeprom_buffer.params.Throttle_Scale_Type == 1){
            write_filled_rectangle_lm(posX-25, posY+10, pos_th_y, 5, 1, 1);
            write_hline_lm(pos_th_x, posX+25, posY+10, 1, 1);
            write_hline_lm(pos_th_x, posX+25, posY+14, 1, 1);
            write_vline_lm(posX+25, posY+10, posY+14, 1, 1);
            write_vline_lm(posX-25, posY+10, posY+14, 1, 1);
        }
    }
}

void hud_draw_CWH(void)
{
    char tmp_str[100] = { 0 };
    float dstlon, dstlat, dstsqrt;

    if((osd_got_home == 0) && (motor_armed) && (osd_fix_type > 1)){
        osd_home_lat = osd_lat;
        osd_home_lon = osd_lon;
        osd_got_home = 1;
    }
    else if(osd_got_home == 1){
        // shrinking factor for longitude going to poles direction
//        float rads = fabs(osd_home_lat) * D2R;
//        double scaleLongDown = Fast_Cos(rads);
//        double scaleLongUp   = 1.0f/Fast_Cos(rads);
        double scaleLongDown = Fast_Cos(fabs(osd_home_lat));
        double scaleLongUp   = 1.0f/Fast_Cos(fabs(osd_home_lat));

        //DST to Home
        dstlat = fabs(osd_home_lat - osd_lat) * 111319.5f;
        dstlon = fabs(osd_home_lon - osd_lon) * 111319.5f * scaleLongDown;
        dstsqrt = dstlat*dstlat + dstlon*dstlon;
        osd_home_distance = sqrt(dstsqrt) / 10000000.0f;

        //DIR to Home
        dstlon = (osd_home_lon - osd_lon); //OffSet_X
        dstlat = (osd_home_lat - osd_lat) * scaleLongUp; //OffSet Y
        osd_home_bearing = 270 + (atan2(dstlat, -dstlon) * R2D); //absolut home direction
        osd_home_bearing = (osd_home_bearing+360)%360;
    }

    //distance
    if(eeprom_buffer.params.CWH_home_dist_en==1 && bShownAtPanle(eeprom_buffer.params.CWH_home_dist_panel)){
        float tmp = osd_home_distance * convert_distance;
        if (tmp < convert_distance_divider)
            sprintf(tmp_str, "H: %d%s", (int)tmp, dist_unit_short);
        else
            sprintf(tmp_str, "H: %0.2f%s", (double)(tmp / convert_distance_divider), dist_unit_long);

        write_string(tmp_str, eeprom_buffer.params.CWH_home_dist_posX, eeprom_buffer.params.CWH_home_dist_posY, 0, 0, TEXT_VA_TOP, eeprom_buffer.params.CWH_home_dist_align, 0, SIZE_TO_FONT[eeprom_buffer.params.CWH_home_dist_fontsize]);
    }
    if((wp_number != 0) &&(eeprom_buffer.params.CWH_wp_dist_en) && bShownAtPanle(eeprom_buffer.params.CWH_wp_dist_panel)){
        float tmp = wp_dist * convert_distance;
        if (tmp < convert_distance_divider)
            sprintf(tmp_str, "WP: %d%s", (int)tmp, dist_unit_short);
        else
            sprintf(tmp_str, "WP: %0.2f%s", (double)(tmp / convert_distance_divider), dist_unit_long);

        write_string(tmp_str, eeprom_buffer.params.CWH_wp_dist_posX, eeprom_buffer.params.CWH_wp_dist_posY, 0, 0, TEXT_VA_TOP, eeprom_buffer.params.CWH_wp_dist_align, 0, SIZE_TO_FONT[eeprom_buffer.params.CWH_wp_dist_fontsize]);
    }

    //direction - map-like mode
    if(eeprom_buffer.params.CWH_Nmode_en==1 && bShownAtPanle(eeprom_buffer.params.CWH_Nmode_panel)){
        hud_draw_head_wp_home();
    }

    //direction - scale mode
    if(eeprom_buffer.params.CWH_Tmode_en==1 && bShownAtPanle(eeprom_buffer.params.CWH_Tmode_panel)){
        hud_draw_linear_compass(osd_heading, 0, 120, 180, GRAPHICS_X_MIDDLE, eeprom_buffer.params.CWH_Tmode_posY, 15, 30, 5, 8, 0);
    }

}

/**
 * hud_draw_compass: Draw a compass.
 *
 * @param       v               value for the compass
 * @param       range           range about value to display (+/- range/2 each direction)
 * @param       width           length in pixels
 * @param       x               x displacement
 * @param       y               y displacement
 * @param       mintick_step    how often a minor tick is shown
 * @param       majtick_step    how often a major tick (heading "xx") is shown
 * @param       mintick_len     minor tick length
 * @param       majtick_len     major tick length
 * @param       flags           special flags (see hud.h.)
 */
#define COMPASS_SMALL_NUMBER
// #define COMPASS_FILLED_NUMBER
void hud_draw_linear_compass(int v, int home_dir, int range, int width, int x, int y, int mintick_step, int majtick_step, int mintick_len, int majtick_len, __attribute__((unused)) int flags)
{
    v %= 360; // wrap, just in case.
    struct FontEntry font_info;
    int majtick_start = 0, majtick_end = 0, mintick_start = 0, mintick_end = 0, textoffset = 0;
    char headingstr[4];
    majtick_start = y;
    majtick_end   = y - majtick_len;
    mintick_start = y;
    mintick_end   = y - mintick_len;
    textoffset    = 8;
    int r, style, rr, xs; // rv,
    int range_2 = range / 2;
    bool home_drawn = false;

//  home_dir = 30;
//  int wp_dir = 60;

    for (r = -range_2; r <= +range_2; r++) {
        style = 0;
        rr    = (v + r + 360) % 360; // normalise range for modulo, add to move compass track
        // rv = -rr + range_2; // for number display
        if (rr % majtick_step == 0) {
            style = 1; // major tick
        } else if (rr % mintick_step == 0) {
            style = 2; // minor tick
        }
        if (style) {
            // Calculate x position.
            xs = ((long int)(r * width) / (long int)range) + x;
            // Draw it.
            if (style == 1) {
                write_vline_outlined(xs, majtick_start, majtick_end, 2, 2, 0, 1);
                // Draw heading above this tick.
                // If it's not one of north, south, east, west, draw the heading.
                // Otherwise, draw one of the identifiers.
                if (rr % 90 != 0) {
                    // We abbreviate heading to two digits. This has the side effect of being easy to compute.
                    headingstr[0] = '0' + (rr / 100);
                    headingstr[1] = '0' + ((rr / 10) % 10);
                    headingstr[2] = 0;
                    headingstr[3] = 0; // nul to terminate
                } else {
                    switch (rr) {
                    case 0:
                        headingstr[0] = 'N';
                        break;
                    case 90:
                        headingstr[0] = 'E';
                        break;
                    case 180:
                        headingstr[0] = 'S';
                        break;
                    case 270:
                        headingstr[0] = 'W';
                        break;
                    }
                    headingstr[1] = 0;
                    headingstr[2] = 0;
                    headingstr[3] = 0;
                }
                // +1 fudge...!
                write_string(headingstr, xs + 1, majtick_start + textoffset, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, 1);

            } else if (style == 2) {
                write_vline_outlined(xs, mintick_start, mintick_end, 2, 2, 0, 1);
            }
        }

//      // Put home direction
//      if (rr == home_dir) {
//          xs = ((long int)(r * width) / (long int)range) + x;
//          write_filled_rectangle_lm(xs - 5, majtick_start + textoffset + 7, 10, 10, 0, 1);
//          write_string("H", xs + 1, majtick_start + textoffset + 12, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, 1);
//          home_drawn = true;
//      }
//
//      // Put home direction
//      if (rr == wp_dir) {
//          xs = ((long int)(r * width) / (long int)range) + x;
//          write_filled_rectangle_lm(xs - 5, majtick_start + textoffset + 7, 10, 10, 0, 1);
//          write_string("W", xs + 1, majtick_start + textoffset + 12, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, 1);
////            home_drawn = true;
//      }
    }

//  if (home_dir > 0 && !home_drawn) {
//      if (((v > home_dir) && (v - home_dir < 180)) || ((v < home_dir) && (home_dir -v > 180)))
//          r = x - ((long int)(range_2 * width) / (long int)range);
//      else
//          r = x + ((long int)(range_2 * width) / (long int)range);

//      write_filled_rectangle_lm(r - 5, majtick_start + textoffset + 7, 10, 10, 0, 1);
//      write_string("H", r + 1, majtick_start + textoffset + 12, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, 1);
//  }


    // Then, draw a rectangle with the present heading in it.
    // We want to cover up any other markers on the bottom.
    // First compute font size.
    headingstr[0] = '0' + (v / 100);
    headingstr[1] = '0' + ((v / 10) % 10);
    headingstr[2] = '0' + (v % 10);
    headingstr[3] = 0;
    fetch_font_info(0, 3, &font_info, NULL);
#ifdef COMPASS_SMALL_NUMBER
    int rect_width = font_info.width * 3;
#ifdef COMPASS_FILLED_NUMBER
    write_filled_rectangle_lm(x - (rect_width / 2), majtick_start - 7, rect_width, font_info.height, 0, 1);
#else
    write_filled_rectangle_lm(x - (rect_width / 2), majtick_start - 7, rect_width, font_info.height, 0, 0);
#endif
    write_rectangle_outlined(x - (rect_width / 2), majtick_start - 7, rect_width, font_info.height, 0, 1);
    write_string(headingstr, x + 1, majtick_start + textoffset - 5, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 1, 0);
#else
    int rect_width = (font_info.width + 1) * 3 + 2;
#ifdef COMPASS_FILLED_NUMBER
    write_filled_rectangle_lm(x - (rect_width / 2), majtick_start + 2, rect_width, font_info.height + 2, 0, 1);
#else
    write_filled_rectangle_lm(x - (rect_width / 2), majtick_start + 2, rect_width, font_info.height + 2, 0, 0);
#endif
    write_rectangle_outlined(x - (rect_width / 2), majtick_start + 2, rect_width, font_info.height + 2, 0, 1);
    write_string(headingstr, x + 1, majtick_start + textoffset + 2, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 1, 3);
#endif
}

/**
 * hud_draw_vertical_scale: Draw a vertical scale.
 *
 * @param       v                   value to display as an integer
 * @param       range               range about value to display (+/- range/2 each direction)
 * @param       halign              horizontal alignment: 0 = left, 1 = right.
 * @param       x                   x displacement
 * @param       y                   y displacement
 * @param       height              height of scale
 * @param       mintick_step        how often a minor tick is shown
 * @param       majtick_step        how often a major tick is shown
 * @param       mintick_len         minor tick length
 * @param       majtick_len         major tick length
 * @param       boundtick_len       boundary tick length
 * @param       max_val             maximum expected value (used to compute size of arrow ticker)
 * @param       flags               special flags (see hud.h.)
 */
// #define VERTICAL_SCALE_BRUTE_FORCE_BLANK_OUT
#define VERTICAL_SCALE_FILLED_NUMBER
void hud_draw_vertical_scale(int v, int range, int halign, int x, int y,
        int height, int mintick_step, int majtick_step, int mintick_len,
        int majtick_len, int boundtick_len, __attribute__((unused)) int max_val,
        int flags)
{
    char temp[15];
    struct FontEntry font_info;
    struct FontDimensions dim;
    // Compute the position of the elements.
    int majtick_start = 0, majtick_end = 0, mintick_start = 0, mintick_end = 0, boundtick_start = 0, boundtick_end = 0;

    majtick_start   = x;
    mintick_start   = x;
    boundtick_start = x;
    if (halign == 0) {
        majtick_end     = x + majtick_len;
        mintick_end     = x + mintick_len;
        boundtick_end   = x + boundtick_len;
    } else if (halign == 1) {
        majtick_end     = x - majtick_len;
        mintick_end     = x - mintick_len;
        boundtick_end   = x - boundtick_len;
    }
    // Retrieve width of large font (font #0); from this calculate the x spacing.
    fetch_font_info(0, 0, &font_info, NULL);
    int arrow_len      = (font_info.height / 2) + 1;
    int text_x_spacing = (font_info.width / 2);
    int max_text_y     = 0, text_length = 0;
    int small_font_char_width = font_info.width + 1; // +1 for horizontal spacing = 1
    // For -(range / 2) to +(range / 2), draw the scale.
    int range_2 = range / 2; // , height_2 = height / 2;
    int r = 0, rr = 0, rv = 0, ys = 0, style = 0; // calc_ys = 0,
    // Iterate through each step.
    for (r = -range_2; r <= +range_2; r++) {
        style = 0;
        rr    = r + range_2 - v; // normalise range for modulo, subtract value to move ticker tape
        rv    = -rr + range_2; // for number display
        if (flags & HUD_VSCALE_FLAG_NO_NEGATIVE) {
            rr += majtick_step / 2;
        }
        if (rr % majtick_step == 0) {
            style = 1; // major tick
        } else if (rr % mintick_step == 0) {
            style = 2; // minor tick
        } else {
            style = 0;
        }
        if (flags & HUD_VSCALE_FLAG_NO_NEGATIVE && rv < 0) {
            continue;
        }
        if (style) {
            // Calculate y position.
            ys = ((long int)(r * height) / (long int)range) + y;
            // Depending on style, draw a minor or a major tick.
            if (style == 1) {
                write_hline_outlined(majtick_start, majtick_end, ys, 2, 2, 0, 1);
                memset(temp, ' ', 10);
                sprintf(temp, "%d", rv);
                text_length = (strlen(temp) + 1) * small_font_char_width; // add 1 for margin
                if (text_length > max_text_y) {
                    max_text_y = text_length;
                }
                if (halign == 0) {
                    write_string(temp, majtick_end + text_x_spacing + 1, ys, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_LEFT, 0, 1);
                } else {
                    write_string(temp, majtick_end - text_x_spacing + 1, ys, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_RIGHT, 0, 1);
                }
            } else if (style == 2) {
                write_hline_outlined(mintick_start, mintick_end, ys, 2, 2, 0, 1);
            }
        }
    }
    // Generate the string for the value, as well as calculating its dimensions.
    memset(temp, ' ', 10);
    // my_itoa(v, temp);
    sprintf(temp, "%d", v);
    // TODO: add auto-sizing.
    calc_text_dimensions(temp, font_info, 1, 0, &dim);
    int xx = 0, i = 0;
    if (halign == 0) {
        xx = majtick_end + text_x_spacing;
    } else {
        xx = majtick_end - text_x_spacing;
    }
    y++;
    // Draw an arrow from the number to the point.
    for (i = 0; i < arrow_len; i++) {
        if (halign == 0) {
            write_pixel_lm(xx - arrow_len + i, y - i - 1, 1, 1);
            write_pixel_lm(xx - arrow_len + i, y + i - 1, 1, 1);
#ifdef VERTICAL_SCALE_FILLED_NUMBER
            write_hline_lm(xx + dim.width - 1, xx - arrow_len + i + 1, y - i - 1, 0, 1);
            write_hline_lm(xx + dim.width - 1, xx - arrow_len + i + 1, y + i - 1, 0, 1);
#else
            write_hline_lm(xx + dim.width - 1, xx - arrow_len + i + 1, y - i - 1, 0, 0);
            write_hline_lm(xx + dim.width - 1, xx - arrow_len + i + 1, y + i - 1, 0, 0);
#endif
        } else {
            write_pixel_lm(xx + arrow_len - i, y - i - 1, 1, 1);
            write_pixel_lm(xx + arrow_len - i, y + i - 1, 1, 1);
#ifdef VERTICAL_SCALE_FILLED_NUMBER
            write_hline_lm(xx - dim.width - 1, xx + arrow_len - i - 1, y - i - 1, 0, 1);
            write_hline_lm(xx - dim.width - 1, xx + arrow_len - i - 1, y + i - 1, 0, 1);
#else
            write_hline_lm(xx - dim.width - 1, xx + arrow_len - i - 1, y - i - 1, 0, 0);
            write_hline_lm(xx - dim.width - 1, xx + arrow_len - i - 1, y + i - 1, 0, 0);
#endif
        }
    }
    if (halign == 0) {
        write_hline_lm(xx, xx + dim.width - 1, y - arrow_len, 1, 1);
        write_hline_lm(xx, xx + dim.width - 1, y + arrow_len - 2, 1, 1);
        write_vline_lm(xx + dim.width - 1, y - arrow_len, y + arrow_len - 2, 1, 1);
    } else {
        write_hline_lm(xx, xx - dim.width - 1, y - arrow_len, 1, 1);
        write_hline_lm(xx, xx - dim.width - 1, y + arrow_len - 2, 1, 1);
        write_vline_lm(xx - dim.width - 1, y - arrow_len, y + arrow_len - 2, 1, 1);
    }
    // Draw the text.
    if (halign == 0) {
        write_string(temp, xx, y, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_LEFT, 0, 0);
    } else {
        write_string(temp, xx, y, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_RIGHT, 0, 0);
    }
#ifdef VERTICAL_SCALE_BRUTE_FORCE_BLANK_OUT
    // This is a bad brute force method destuctive to other things that maybe drawn underneath like e.g. the artificial horizon:
    // Then, add a slow cut off on the edges, so the text doesn't sharply
    // disappear. We simply clear the areas above and below the ticker, and we
    // use little markers on the edges.
    if (halign == 0) {
        write_filled_rectangle_lm(majtick_end + text_x_spacing, y + (height / 2) - (font_info.height / 2), max_text_y - boundtick_start, font_info.height, 0, 0);
        write_filled_rectangle_lm(majtick_end + text_x_spacing, y - (height / 2) - (font_info.height / 2), max_text_y - boundtick_start, font_info.height, 0, 0);
    } else {
        write_filled_rectangle_lm(majtick_end - text_x_spacing - max_text_y, y + (height / 2) - (font_info.height / 2), max_text_y, font_info.height, 0, 0);
        write_filled_rectangle_lm(majtick_end - text_x_spacing - max_text_y, y - (height / 2) - (font_info.height / 2), max_text_y, font_info.height, 0, 0);
    }
#endif
    y--;
    write_hline_outlined(boundtick_start, boundtick_end, y + (height / 2), 2, 2, 0, 1);
    write_hline_outlined(boundtick_start, boundtick_end, y - (height / 2), 2, 2, 0, 1);
}

void hud_draw_head_wp_home()
{
    int posX, posY, r;
    char tmp_str[10] = {0};

    //draw compass
    posX = eeprom_buffer.params.CWH_Nmode_posX;
    posY = eeprom_buffer.params.CWH_Nmode_posY;
    r = eeprom_buffer.params.CWH_Nmode_radius;
    write_circle_outlined(posX, posY, r, 0, 1, 0, 1);
//    write_string("N", posX, posY - r, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, 0, SIZE_TO_FONT[0]);

    //draw heading
    POLYGON2D suav;
    suav.state       = 1;
    suav.num_verts   = 3;
    suav.x0          = posX;
    suav.y0          = posY;
    VECTOR2D_INITXYZ(&(suav.vlist_local[0]), 0, -7);
    VECTOR2D_INITXYZ(&(suav.vlist_local[1]), -3, 7);
    VECTOR2D_INITXYZ(&(suav.vlist_local[2]), 3, 7);
    Reset_Polygon2D(&suav);
    Rotate_Polygon2D(&suav, osd_heading);
    write_line_outlined(suav.vlist_trans[0].x+suav.x0, suav.vlist_trans[0].y+suav.y0,
                        suav.vlist_trans[1].x+suav.x0,suav.vlist_trans[1].y+suav.y0, 2, 2, 0, 1);
    write_line_outlined(suav.vlist_trans[0].x+suav.x0, suav.vlist_trans[0].y+suav.y0,
                        suav.vlist_trans[2].x+suav.x0,suav.vlist_trans[2].y+suav.y0, 2, 2, 0, 1);

    // draw home
    // the home only shown when the distance above 1m
    if(((int32_t)osd_home_distance > 1))
    {
        float homeCX = posX + (eeprom_buffer.params.CWH_Nmode_home_radius)*Fast_Sin(osd_home_bearing);
        float homeCY = posY - (eeprom_buffer.params.CWH_Nmode_home_radius)*Fast_Cos(osd_home_bearing);
        write_string("H", homeCX, homeCY, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, SIZE_TO_FONT[0]);
    }

    //draw waypoint
    if((wp_number != 0) && (wp_dist > 1))
    {
        //format bearing
        wp_target_bearing = (wp_target_bearing + 360)%360;
        float wpCX = posX + (eeprom_buffer.params.CWH_Nmode_wp_radius)*Fast_Sin(wp_target_bearing);
        float wpCY = posY - (eeprom_buffer.params.CWH_Nmode_wp_radius)*Fast_Cos(wp_target_bearing);
        sprintf(tmp_str, "%d", (int)wp_number);
        write_string(tmp_str, wpCX, wpCY, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, SIZE_TO_FONT[0]);
    }
}

void hud_draw_wind(void)
{
    char tmp_str[10] = {0};
    uint16_t posX = eeprom_buffer.params.Wind_posX;
    uint16_t posY = eeprom_buffer.params.Wind_posY;

    //write_string("wind:", posX, posY, 0, 0, TEXT_VA_MIDDLE, eeprom_buffer.params.Wind_align, 0, SIZE_TO_FONT[eeprom_buffer.params.Wind_fontsize]);

    //draw direction
    POLYGON2D obj2D;
    obj2D.state       = 1;
    obj2D.num_verts   = 5;
    obj2D.x0          = posX;
    obj2D.y0          = posY;
    VECTOR2D_INITXYZ(&(obj2D.vlist_local[0]), -3, -2);
    VECTOR2D_INITXYZ(&(obj2D.vlist_local[1]), 0, -8);
    VECTOR2D_INITXYZ(&(obj2D.vlist_local[2]), 3, -2);
    VECTOR2D_INITXYZ(&(obj2D.vlist_local[3]), 0, 8);
    VECTOR2D_INITXYZ(&(obj2D.vlist_local[4]), 0, -2);
    Reset_Polygon2D(&obj2D);
    Rotate_Polygon2D(&obj2D, osd_windDir);
    write_triangle_wire(obj2D.vlist_trans[0].x+obj2D.x0, obj2D.vlist_trans[0].y+obj2D.y0,
                                            obj2D.vlist_trans[1].x+obj2D.x0,obj2D.vlist_trans[1].y+obj2D.y0,
                                            obj2D.vlist_trans[2].x+obj2D.x0, obj2D.vlist_trans[2].y+obj2D.y0);
    write_line_outlined(obj2D.vlist_trans[3].x+obj2D.x0, obj2D.vlist_trans[3].y+obj2D.y0,
                                            obj2D.vlist_trans[4].x+obj2D.x0,obj2D.vlist_trans[4].y+obj2D.y0, 2, 2, 0, 1);

    //draw wind speed
    float tmp = osd_windSpeed * convert_speed;
    sprintf(tmp_str, "%.2f%s", tmp, spd_unit);
    write_string(tmp_str, posX+15, posY, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_LEFT, 0, SIZE_TO_FONT[0]);
}

void hud_draw_warnning(void)
{

    write_string(warn_str, eeprom_buffer.params.Alarm_posX, eeprom_buffer.params.Alarm_posY, 0, 0, TEXT_VA_TOP, eeprom_buffer.params.Alarm_align, 0, SIZE_TO_FONT[eeprom_buffer.params.Alarm_fontsize]);

    if((GetSystimeMS() - last_warn_time) < 1000)
    {
        return;
    }


    bool haswarn = false;
    const static int warn_cnt = 6;
    uint8_t warning[]={0, 0, 0, 0, 0, 0};

    //no GPS fix!
    if( eeprom_buffer.params.Alarm_GPS_status_en==1 && (osd_fix_type < GPS_OK_FIX_3D)) {
        haswarn = true;
        warning[0] = 1;
    }

    //low batt
    if( eeprom_buffer.params.Alarm_low_batt_en==1 && (osd_battery_remaining_A < eeprom_buffer.params.Alarm_low_batt)) {
        haswarn = true;
        warning[1] = 1;
    }

    //under speed
    if( eeprom_buffer.params.Alarm_low_speed_en==1 && (osd_groundspeed < eeprom_buffer.params.Alarm_low_speed)) {
        haswarn = true;
        warning[2] = 1;
    }

    //over speed
    if( eeprom_buffer.params.Alarm_over_speed_en==1 && (osd_groundspeed > eeprom_buffer.params.Alarm_over_speed)) {
        haswarn = true;
        warning[3] = 1;
    }

    //under altitude
    if( eeprom_buffer.params.Alarm_low_alt_en==1 && (osd_alt < eeprom_buffer.params.Alarm_low_alt)) {
        haswarn = true;
        warning[4] = 1;
    }

    //over altitude
    if( eeprom_buffer.params.Alarm_over_alt_en==1 && (osd_alt > eeprom_buffer.params.Alarm_over_alt)) {
        haswarn = true;
        warning[5] = 1;
    }

    if(haswarn ){
        last_warn_time = GetSystimeMS();
        if(last_warn_type > (warn_cnt - 1)) last_warn_type = 0;

        if((last_warn_type == 0) && (warning[0] == 1)){
            warn_str = "NO GPS FIX";
            last_warn_type++;
            return;
        }

        if((last_warn_type == 1) && (warning[1] == 1)){
            warn_str = "LOW BATTERY";
            last_warn_type++;
            return;
        }

        if((last_warn_type == 2) && (warning[2] == 1)){
            warn_str = "SPEED LOW";
            last_warn_type++;
            return;
        }
        if((last_warn_type == 3) && (warning[3] == 1)){
            warn_str = "OVER SPEED";
            last_warn_type++;
            return;
        }
        if((last_warn_type == 4) && (warning[4] == 1)){
            warn_str = "LOW ALT";
            last_warn_type++;
            return;
        }
        if((last_warn_type == 5) && (warning[5] == 1)){
            warn_str = "HIGH ALT";
            last_warn_type++;
            return;
        }
        last_warn_type++;
    }
    else{
        warn_str = "";
    }
}

void DJI_test(void)
{
    char tmp_str[100] = { 0 };

    int16_t tmp_int16;
    int tmp_int1, tmp_int2;

    sprintf(tmp_str, "%0.1fV", (float)osd_vbat_A);
    write_string(tmp_str, 20, 50, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, SIZE_TO_FONT[1]);
    sprintf(tmp_str, "%i", osd_battery_remaining_A);
    write_string(tmp_str, 90, 50, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[1]);

    sprintf(tmp_str, "P %d", (int32_t)osd_pitch);
    write_string(tmp_str, GRAPHICS_X_MIDDLE-50, 50, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, SIZE_TO_FONT[1]);
    sprintf(tmp_str, "R %d", (int32_t)osd_roll);
    write_string(tmp_str, GRAPHICS_X_MIDDLE-50, 65, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, SIZE_TO_FONT[1]);

    sprintf(tmp_str, "hV %0.1f", (double)osd_groundspeed);
    write_string(tmp_str, GRAPHICS_X_MIDDLE+10, 50, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, SIZE_TO_FONT[1]);

    sprintf(tmp_str, "GPS %d", osd_satellites_visible);
    write_string(tmp_str, GRAPHICS_X_MIDDLE+150, 50, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[1]);
    sprintf(tmp_str, "%0.5f lat", (double)(osd_lat / 10000000.0));
    write_string(tmp_str, GRAPHICS_X_MIDDLE+170, 65, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[1]);
    sprintf(tmp_str, "%0.5f lon", (double)(osd_lon / 10000000.0));
    write_string(tmp_str, GRAPHICS_X_MIDDLE+170, 80, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[1]);
    sprintf(tmp_str, "%0.1f hdop", (double)(osd_hdop));
    write_string(tmp_str, GRAPHICS_X_MIDDLE+170, 95, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[1]);

    sprintf(tmp_str, "D %d", (int32_t)osd_home_distance);
    write_string(tmp_str, 20, GRAPHICS_Y_MIDDLE-30, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, SIZE_TO_FONT[1]);

    sprintf(tmp_str, "H %d", (int32_t)osd_alt);
    write_string(tmp_str, 20, GRAPHICS_Y_MIDDLE-15, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, SIZE_TO_FONT[1]);

    char* mode_str = "unknown";
    if(osd_mode == 0) mode_str = "MANUAL";
    else if(osd_mode == 1) mode_str = "GPS";
    else if(osd_mode == 2) mode_str = "FAILSAFE";
    else if(osd_mode == 3) mode_str = "ATTI";

    write_string(mode_str, 20, GRAPHICS_Y_MIDDLE+15, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, SIZE_TO_FONT[0]);

    sprintf(tmp_str, "%0.5f Hlat", (double)(osd_home_lat / 10000000.0));
    write_string(tmp_str, GRAPHICS_X_MIDDLE+10, GRAPHICS_Y_MIDDLE+15, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[1]);
    sprintf(tmp_str, "%0.5f Hlon", (double)(osd_home_lon / 10000000.0));
    write_string(tmp_str, GRAPHICS_X_MIDDLE+170, GRAPHICS_Y_MIDDLE+15, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[1]);

    sprintf(tmp_str, "vV %0.1f", (double)osd_downVelocity);
    write_string(tmp_str, GRAPHICS_X_MIDDLE+150, GRAPHICS_Y_MIDDLE-15, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[1]);

    hud_draw_linear_compass(osd_heading, osd_home_bearing, 120, 180, GRAPHICS_X_MIDDLE, GRAPHICS_Y_MIDDLE+80, 15, 30, 5, 8, 0);
}
