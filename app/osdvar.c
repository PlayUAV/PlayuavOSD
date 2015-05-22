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

#include "osdvar.h"

/////////////////////////////////////////////////////////////////////////
u8		mavbeat = 0;
u32 	lastMAVBeat = 0;
u32		lastWritePanel = 0;
u8		waitingMAVBeats = 1;
u8		apm_mav_type;
u8		apm_mav_system; 
u8		apm_mav_component;
u8		enable_mav_request = 0;
u32 	sys_start_time = 0;

/////////////////////////////////////////////////////////////////////////
float        osd_vbat_A = 0;                 // Battery A voltage in milivolt
int16_t      osd_curr_A = 0;                 // Battery A current
int8_t       osd_battery_remaining_A = 0;    // 0 to 100 <=> 0 to 1000

float       osd_pitch = 0;                  // pitch from DCM
float       osd_roll = 0;                   // roll from DCM
float       osd_yaw = 0;                    // relative heading form DCM
float        osd_heading = 0;                // ground course heading from GPS

float        osd_lat = 0;                    // latidude
float        osd_lon = 0;                    // longitude
uint8_t      osd_satellites_visible = 0;     // number of satelites
uint8_t      osd_fix_type = 0;               // GPS lock 0-1=no fix, 2=2D, 3=3D
double 		 osd_hdop = 0.0;

float        osd_lat2 = 0;                    // latidude
float        osd_lon2 = 0;                    // longitude
uint8_t      osd_satellites_visible2 = 0;     // number of satelites
uint8_t      osd_fix_type2 = 0;               // GPS lock 0-1=no fix, 2=2D, 3=3D
double 		 osd_hdop2 = 0.0;

float        osd_airspeed = -1;              // airspeed
float        osd_groundspeed = 0;            // ground speed
float        osd_downVelocity = 0;
uint16_t     osd_throttle = 0;               // throtle
float        osd_alt = 0;                    // altitude
float        osd_climb = 0;

float	    nav_roll = 0; // Current desired roll in degrees
float        nav_pitch = 0; // Current desired pitch in degrees
int16_t	    nav_bearing = 0; // Current desired heading in degrees
int16_t	    wp_target_bearing = 0; // Bearing to current MISSION/target in degrees
int8_t       wp_target_bearing_rotate_int = 0;
uint16_t     wp_dist = 0; // Distance to active MISSION in meters
uint8_t      wp_number = 0; // Current waypoint number
float	    alt_error = 0; // Current altitude error in meters
float        aspd_error = 0; // Current airspeed error in meters/second
float	    xtrack_error = 0; // Current crosstrack error on x-y plane in meters
float        eff = 0; //Efficiency

uint8_t      osd_mode = 0;
bool 		 motor_armed = 0;
uint8_t      base_mode = 0;

int16_t      chan1_raw = 0;
int16_t      chan2_raw = 0;
uint16_t     osd_chan5_raw = 1000;
uint16_t     osd_chan6_raw = 1000;
uint16_t     osd_chan7_raw = 1000;
uint16_t     osd_chan8_raw = 1000;
uint8_t      osd_rssi = 0; //raw value from mavlink

uint8_t      osd_got_home = 0;               // tels if got home position or not
float        osd_home_lat = 0;               // home latidude
float        osd_home_lon = 0;               // home longitude
float        osd_home_alt = 0; 
long         osd_home_distance = 0;          // distance from home
uint32_t	 osd_home_bearing = 0;
uint8_t      osd_alt_cnt = 0;              // counter for stable osd_alt
float        osd_alt_prev = 0;             // previous altitude

volatile uint8_t 	current_panel = 1;


