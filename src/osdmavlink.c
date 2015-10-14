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
 * MinimOSD - arducam-osd Controller(https://code.google.com/p/arducam-osd/)
 */

#include "osdmavlink.h"
#include "osdvar.h"

#define MAX_STREAMS 6

mavlink_system_t mavlink_system = {12,1,0,0}; //modified
mavlink_message_t msg; 
mavlink_status_t status;
uint8_t mavlink_active = 0;
uint8_t mavlink_requested = 0;

extern xSemaphoreHandle onMavlinkSemaphore;
extern uint8_t *mavlink_buffer_proc;

void request_mavlink_rates(void)
{
    const u8 MAVStreams[MAX_STREAMS] = {MAV_DATA_STREAM_RAW_SENSORS,
        MAV_DATA_STREAM_EXTENDED_STATUS,
        MAV_DATA_STREAM_RC_CHANNELS,
        MAV_DATA_STREAM_POSITION,
        MAV_DATA_STREAM_EXTRA1, 
        MAV_DATA_STREAM_EXTRA2};
    //const u16 MAVRates[maxStreams] = {0x01, 0x02, 0x05, 0x02, 0x05, 0x02};
	uint16_t MAVRates[MAX_STREAMS] = {0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A};

	if(apm_mav_component == 0x32) //pixhawk origin FW
	{
		return; //we need not change the rate
	}
    for (uint32_t i=0; i < MAX_STREAMS; i++) {
        mavlink_msg_request_data_stream_send(MAVLINK_COMM_0,
            apm_mav_system, apm_mav_component,
            MAVStreams[i], MAVRates[i], 1);
    }
}

void request_mission_count(void)
{
    mavlink_msg_mission_request_list_send(MAVLINK_COMM_0, 
                    apm_mav_system, apm_mav_component);
}

void request_mission_item(uint16_t index)
{
    mavlink_msg_mission_request_send(MAVLINK_COMM_0, 
                    apm_mav_system, apm_mav_component, index);
}

void parseMavlink(void)
{
    uint8_t c;
    uint8_t index = 0;

    while (index < MAVLINK_BUFFER_SIZE)
    {
        c = mavlink_buffer_proc[index++];
        if(mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status))
        {
            mavlink_active = 1;
            //handle msg
            switch(msg.msgid)
            {
                case MAVLINK_MSG_ID_HEARTBEAT:
                    {
                        mavbeat = 1;
                        apm_mav_system    = msg.sysid;
                        apm_mav_component = msg.compid;
                        apm_mav_type      = mavlink_msg_heartbeat_get_type(&msg);
                        osd_mode = mavlink_msg_heartbeat_get_custom_mode(&msg);
                        base_mode = mavlink_msg_heartbeat_get_base_mode(&msg);

                        last_motor_armed = motor_armed;
                        motor_armed = base_mode & (1 << 7);

                        if(heatbeat_start_time == 0){
                            heatbeat_start_time = GetSystimeMS();
                        }

                        if (!last_motor_armed && motor_armed) {
                            armed_start_time = GetSystimeMS();
                        }

                        if (last_motor_armed && !motor_armed){
                            total_armed_time = GetSystimeMS() - armed_start_time + total_armed_time;
                            armed_start_time = 0;
                        }

                        lastMAVBeat = GetSystimeMS();
                        if(waitingMAVBeats == 1){
                            enable_mav_request = 1;
                        }
                        
                        if((got_mission_counts == 0) && (enable_mission_count_request == 0))
                        {
                            enable_mission_count_request = 1;
                        }
                    }
                    break;
                case MAVLINK_MSG_ID_SYS_STATUS:
                    {
                        osd_vbat_A = (mavlink_msg_sys_status_get_voltage_battery(&msg) / 1000.0f); //Battery voltage, in millivolts (1 = 1 millivolt)
                        osd_curr_A = mavlink_msg_sys_status_get_current_battery(&msg); //Battery current, in 10*milliamperes (1 = 10 milliampere)
                        osd_battery_remaining_A = mavlink_msg_sys_status_get_battery_remaining(&msg); //Remaining battery energy: (0%: 0, 100%: 100)
                        //osd_mode = apm_mav_component;//Debug
                        //osd_nav_mode = apm_mav_system;//Debug
                    }
                    break;
                case MAVLINK_MSG_ID_GPS_RAW_INT:
                    {
                        osd_lat = mavlink_msg_gps_raw_int_get_lat(&msg);
                        osd_lon = mavlink_msg_gps_raw_int_get_lon(&msg);
                        osd_fix_type = mavlink_msg_gps_raw_int_get_fix_type(&msg);
                        osd_hdop = mavlink_msg_gps_raw_int_get_eph(&msg);
                        osd_satellites_visible = mavlink_msg_gps_raw_int_get_satellites_visible(&msg);
                    }
                    break;
                case MAVLINK_MSG_ID_GPS2_RAW:
                    {
                        osd_lat2 = mavlink_msg_gps2_raw_get_lat(&msg);
                        osd_lon2 = mavlink_msg_gps2_raw_get_lon(&msg);
                        osd_fix_type2 = mavlink_msg_gps2_raw_get_fix_type(&msg);
                        osd_hdop2 = mavlink_msg_gps2_raw_get_eph(&msg);
                        osd_satellites_visible2 = mavlink_msg_gps2_raw_get_satellites_visible(&msg);
                    }
                    break;
                case MAVLINK_MSG_ID_VFR_HUD:
                    {
                        osd_airspeed = mavlink_msg_vfr_hud_get_airspeed(&msg);
                        osd_groundspeed = mavlink_msg_vfr_hud_get_groundspeed(&msg);
                        osd_heading = mavlink_msg_vfr_hud_get_heading(&msg); // 0..360 deg, 0=north
                        osd_throttle = mavlink_msg_vfr_hud_get_throttle(&msg);
                        //if(osd_throttle > 100 && osd_throttle < 150) osd_throttle = 100;//Temporary fix for ArduPlane 2.28
                        //if(osd_throttle < 0 || osd_throttle > 150) osd_throttle = 0;//Temporary fix for ArduPlane 2.28
                        osd_alt = mavlink_msg_vfr_hud_get_alt(&msg);
                        osd_climb = mavlink_msg_vfr_hud_get_climb(&msg);
                    }
                    break;
                case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:              // jmmods 
                    {
                        osd_rel_alt = mavlink_msg_global_position_int_get_relative_alt(&msg) / 1000.0 ;   // jmmods 
                        // uav.relative_alt = packet.relative_alt / 1000.0; //jmtune Altitude above ground in meters, expressed as * 1000 (millimeters)   // jmmods
                    }
                     break;
                
                case MAVLINK_MSG_ID_ATTITUDE:
                    {
                        osd_pitch = Rad2Deg(mavlink_msg_attitude_get_pitch(&msg));
                        osd_roll = Rad2Deg(mavlink_msg_attitude_get_roll(&msg));
                        osd_yaw = Rad2Deg(mavlink_msg_attitude_get_yaw(&msg));
                    }
                    break;
                case MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT:
                    {
                        nav_roll = mavlink_msg_nav_controller_output_get_nav_roll(&msg);
                        nav_pitch = mavlink_msg_nav_controller_output_get_nav_pitch(&msg);
                        nav_bearing = mavlink_msg_nav_controller_output_get_nav_bearing(&msg);
                        wp_target_bearing = mavlink_msg_nav_controller_output_get_target_bearing(&msg);
                        wp_dist = mavlink_msg_nav_controller_output_get_wp_dist(&msg);
                        alt_error = mavlink_msg_nav_controller_output_get_alt_error(&msg);
                        aspd_error = mavlink_msg_nav_controller_output_get_aspd_error(&msg);
                        xtrack_error = mavlink_msg_nav_controller_output_get_xtrack_error(&msg);
                    }
                    break;
                case MAVLINK_MSG_ID_MISSION_CURRENT:
                    {
                        wp_number = (uint8_t)mavlink_msg_mission_current_get_seq(&msg);
                    }
                    break;
                case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
                    {
                        if(!osd_chan_cnt_above_eight)
                        {
                            osd_chan1_raw = mavlink_msg_rc_channels_raw_get_chan1_raw(&msg);
                            osd_chan2_raw = mavlink_msg_rc_channels_raw_get_chan2_raw(&msg);
                            osd_chan3_raw = mavlink_msg_rc_channels_raw_get_chan3_raw(&msg);
                            osd_chan4_raw = mavlink_msg_rc_channels_raw_get_chan4_raw(&msg);
                            osd_chan5_raw = mavlink_msg_rc_channels_raw_get_chan5_raw(&msg);
                            osd_chan6_raw = mavlink_msg_rc_channels_raw_get_chan6_raw(&msg);
                            osd_chan7_raw = mavlink_msg_rc_channels_raw_get_chan7_raw(&msg);
                            osd_chan8_raw = mavlink_msg_rc_channels_raw_get_chan8_raw(&msg);
                            osd_rssi = mavlink_msg_rc_channels_raw_get_rssi(&msg);
                        }
                    }
                    break;
                case MAVLINK_MSG_ID_RC_CHANNELS:
                    {
                        osd_chan_cnt_above_eight = true;
                        osd_chan1_raw = mavlink_msg_rc_channels_get_chan1_raw(&msg);
                        osd_chan2_raw = mavlink_msg_rc_channels_get_chan2_raw(&msg);
                        osd_chan3_raw = mavlink_msg_rc_channels_get_chan3_raw(&msg);
                        osd_chan4_raw = mavlink_msg_rc_channels_get_chan4_raw(&msg);
                        osd_chan5_raw = mavlink_msg_rc_channels_get_chan5_raw(&msg);
                        osd_chan6_raw = mavlink_msg_rc_channels_get_chan6_raw(&msg);
                        osd_chan7_raw = mavlink_msg_rc_channels_get_chan7_raw(&msg);
                        osd_chan8_raw = mavlink_msg_rc_channels_get_chan8_raw(&msg);
                        osd_chan9_raw = mavlink_msg_rc_channels_get_chan9_raw(&msg);
                        osd_chan10_raw = mavlink_msg_rc_channels_get_chan10_raw(&msg);
                        osd_chan11_raw = mavlink_msg_rc_channels_get_chan11_raw(&msg);
                        osd_chan12_raw = mavlink_msg_rc_channels_get_chan12_raw(&msg);
                        osd_chan13_raw = mavlink_msg_rc_channels_get_chan13_raw(&msg);
                        osd_chan14_raw = mavlink_msg_rc_channels_get_chan14_raw(&msg);
                        osd_chan15_raw = mavlink_msg_rc_channels_get_chan15_raw(&msg);
                        osd_chan16_raw = mavlink_msg_rc_channels_get_chan16_raw(&msg);
                        osd_rssi = mavlink_msg_rc_channels_get_rssi(&msg);
                    }
                    break;
                case MAVLINK_MSG_ID_WIND:
                    {
                        osd_windDir = mavlink_msg_wind_get_direction(&msg); // 0..360 deg, 0=north
                        osd_windSpeed = mavlink_msg_wind_get_speed(&msg); //m/s
                    }
                    break;

                case MAVLINK_MSG_ID_MISSION_COUNT:
                    {
                        mission_counts = mavlink_msg_mission_count_get_count(&msg);                        
                        got_mission_counts = 1;
                        enable_mission_item_request = 1;
                        current_mission_item_req_index = 0;
                        wp_counts = 0;
                    }
                    break;
                    
                case MAVLINK_MSG_ID_MISSION_ITEM:
                    {
                        uint16_t seq, cmd;
                        
                        seq = mavlink_msg_mission_item_get_seq(&msg);
                        cmd = mavlink_msg_mission_item_get_command(&msg);
                        
                        // received a packet, but not what we requested
                        if(current_mission_item_req_index == seq)
                        {
                            //store the waypoints
                            if((cmd == 16) && (wp_counts < MAX_WAYPOINTS))
                            {
                                
                                wp_list[wp_counts].seq = seq;
                                wp_list[wp_counts].cmd = cmd;
                                
                                wp_list[wp_counts].x = mavlink_msg_mission_item_get_x(&msg);
                                wp_list[wp_counts].y = mavlink_msg_mission_item_get_y(&msg);
                                wp_list[wp_counts].z = mavlink_msg_mission_item_get_z(&msg);

                                wp_list[wp_counts].current = mavlink_msg_mission_item_get_current(&msg);
                                wp_counts++;
                            }
                            
                            current_mission_item_req_index++;
                            if(current_mission_item_req_index >= mission_counts){
                                enable_mission_item_request = 0;
                                got_all_wps = 1;
                            }
                        }
                        

                        
                    }
                    break;

/*
                // will be used in the future. See Samuel's PR:https://github.com/PlayUAV/PlayuavOSD/pull/13
                // Noticed: the type of variable in this message is int32_t. Currently we use float type to simulate.
                case MAVLINK_MSG_ID_BATTERY_STATUS:
                   {
                       osd_battery_consumed_in_mah = mavlink_msg_battery_status_get_current_consumed(&msg);
                   }
                   break;
*/
                default:
                    //Do nothing
                    break;
            } //end switch(msg.msgid)
        }
    }
}

void MavlinkTask(void *pvParameters)
{
    mavlink_usart_init(57600);  // jmmods 19200 for ultimate lrs use
    sys_start_time = GetSystimeMS();

    while (1)
    {
        xSemaphoreTake(onMavlinkSemaphore, portMAX_DELAY);
        parseMavlink();
    }
}
