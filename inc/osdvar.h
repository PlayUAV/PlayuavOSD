#ifndef __OSDVAR_H
#define __OSDVAR_H 	

#include "board.h"


/////////////////////////////////////////////////////////////////////////
extern u8		mavbeat;
extern u32  	lastMAVBeat;
extern u32		lastWritePanel;
extern u8		waitingMAVBeats;
extern u8		apm_mav_type;
extern u8		apm_mav_system; 
extern u8		apm_mav_component;
extern u8		enable_mav_request;
extern u32 		sys_start_time;

/////////////////////////////////////////////////////////////////////////
extern float        osd_vbat_A;                 // Battery A voltage in milivolt
extern int16_t      osd_curr_A;                 // Battery A current
extern int8_t       osd_battery_remaining_A;    // 0 to 100 <=> 0 to 1000

extern float       osd_pitch;                  // pitch from DCM
extern float       osd_roll;                   // roll from DCM
extern float       osd_yaw;                    // relative heading form DCM
extern float        osd_heading;                // ground course heading from GPS

extern float        osd_lat;                    // latidude
extern float        osd_lon;                    // longitude
extern uint8_t      osd_satellites_visible;     // number of satelites
extern uint8_t      osd_fix_type;               // GPS lock 0-1=no fix, 2=2D, 3=3D
extern double 		osd_hdop;				

extern float        osd_lat2;                    // latidude
extern float        osd_lon2;                    // longitude
extern uint8_t      osd_satellites_visible2;     // number of satelites
extern uint8_t      osd_fix_type2;               // GPS lock 0-1=no fix, 2=2D, 3=3D
extern double 		osd_hdop2;				

extern float        osd_airspeed;              // airspeed
extern float        osd_groundspeed;            // ground speed
extern float        osd_downVelocity;            // ground speed
extern uint16_t     osd_throttle;               // throtle
extern float        osd_alt;                    // altitude
extern float        osd_climb;

extern float	    nav_roll; // Current desired roll in degrees
extern float        nav_pitch; // Current desired pitch in degrees
extern int16_t	    nav_bearing; // Current desired heading in degrees
extern int16_t	    wp_target_bearing; // Bearing to current MISSION/target in degrees
extern int8_t       wp_target_bearing_rotate_int;
extern uint16_t     wp_dist; // Distance to active MISSION in meters
extern uint8_t      wp_number; // Current waypoint number
extern float	    alt_error; // Current altitude error in meters
extern float        aspd_error; // Current airspeed error in meters/second
extern float	    xtrack_error; // Current crosstrack error on x-y plane in meters
extern float        eff; //Efficiency

extern uint8_t      osd_mode;
extern bool 		motor_armed;
extern uint8_t      base_mode;

extern int16_t      chan1_raw;
extern int16_t      chan2_raw;
extern uint16_t     osd_chan5_raw;
extern uint16_t     osd_chan6_raw;
extern uint16_t     osd_chan7_raw;
extern uint16_t     osd_chan8_raw;
extern uint8_t      osd_rssi; //raw value from mavlink

extern uint8_t      osd_got_home;               // tels if got home position or not
extern float        osd_home_lat;               // home latidude
extern float        osd_home_lon;               // home longitude
extern float        osd_home_alt; 
extern long         osd_home_distance;          // distance from home
extern uint32_t		osd_home_bearing;
extern uint8_t      osd_alt_cnt;              // counter for stable osd_alt
extern float        osd_alt_prev;             // previous altitude

extern volatile uint8_t 	current_panel;

#endif
