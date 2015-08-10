#ifndef __OSD_CONFIG_H
#define __OSD_CONFIG_H

#include "board.h"

#define EEPROM_START_ADDR 		0x080E0000		//Base @ of Sector 11
#define EERROM_SIZE				1024

void vTaskVCP(void *pvParameters);

void LoadParams(void);
bool StoreParams(void);
bool flash_write_word(uint32_t add, uint32_t value);

typedef union{
	uint8_t		c[EERROM_SIZE];
	uint16_t	s[EERROM_SIZE/2];
	uint32_t    w[EERROM_SIZE/4];
	struct PARAM_TYP
	{
		uint16_t Arm_en;
		uint16_t Arm_panel;
		uint16_t Arm_posX;
		uint16_t Arm_posY;
		uint16_t Arm_fontsize;
		uint16_t Arm_align;						
		
		uint16_t BattVolt_en;
		uint16_t BattVolt_panel;
		uint16_t BattVolt_posX;
		uint16_t BattVolt_posY;
		uint16_t BattVolt_fontsize;
		uint16_t BattVolt_align;		
		
		uint16_t BattCurrent_en;
		uint16_t BattCurrent_panel;
		uint16_t BattCurrent_posX;
		uint16_t BattCurrent_posY;
		uint16_t BattCurrent_fontsize;
		uint16_t BattCurrent_align;			
		
		uint16_t BattConsumed_en;
		uint16_t BattConsumed_panel;
		uint16_t BattConsumed_posX;
		uint16_t BattConsumed_posY;
		uint16_t BattConsumed_fontsize;
		uint16_t BattConsumed_align;		
		uint16_t BattConsumed_InMah;
		
		uint16_t FlightMode_en;
		uint16_t FlightMode_panel;
		uint16_t FlightMode_posX;
		uint16_t FlightMode_posY;
		uint16_t FlightMode_fontsize;
		uint16_t FlightMode_align;			
		
		uint16_t GpsStatus_en;
		uint16_t GpsStatus_panel;
		uint16_t GpsStatus_posX;
		uint16_t GpsStatus_posY;
		uint16_t GpsStatus_fontsize;
		uint16_t GpsStatus_align;			
		
		uint16_t GpsHDOP_en;
		uint16_t GpsHDOP_panel;
		uint16_t GpsHDOP_posX;
		uint16_t GpsHDOP_posY;
		uint16_t GpsHDOP_fontsize;
		uint16_t GpsHDOP_align;
		
		uint16_t GpsLat_en;
		uint16_t GpsLat_panle;
		uint16_t GpsLat_posX;
		uint16_t GpsLat_posY;
		uint16_t GpsLat_fontsize;
		uint16_t GpsLat_align;				
		
		uint16_t GpsLon_en;
		uint16_t GpsLon_panel;
		uint16_t GpsLon_posX;
		uint16_t GpsLon_posY;
		uint16_t GpsLon_fontsize;
		uint16_t GpsLon_align;				
		
		uint16_t Gps2Status_en;
		uint16_t Gps2Status_panel;
		uint16_t Gps2Status_posX;
		uint16_t Gps2Status_posY;
		uint16_t Gps2Status_fontsize;
		uint16_t Gps2Status_align;			
		
		uint16_t Gps2HDOP_en;
		uint16_t Gps2HDOP_panel;
		uint16_t Gps2HDOP_posX;
		uint16_t Gps2HDOP_posY;
		uint16_t Gps2HDOP_fontsize;
		uint16_t Gps2HDOP_align;
		
		uint16_t Gps2Lat_en;
		uint16_t Gps2Lat_panel;
		uint16_t Gps2Lat_posX;
		uint16_t Gps2Lat_posY;
		uint16_t Gps2Lat_fontsize;
		uint16_t Gps2Lat_align;				
		
		uint16_t Gps2Lon_en;
		uint16_t Gps2Lon_panel;
		uint16_t Gps2Lon_posX;
		uint16_t Gps2Lon_posY;
		uint16_t Gps2Lon_fontsize;
		uint16_t Gps2Lon_align;	
		
		uint16_t Time_en;
		uint16_t Time_panel;
		uint16_t Time_posX;
		uint16_t Time_posY;
		uint16_t Time_fontsize;
		uint16_t Time_align;				
		
		uint16_t TALT_en;
		uint16_t TALT_panel;
		uint16_t TALT_posX;	
		uint16_t TALT_posY;
		uint16_t TALT_fontsize;
		uint16_t TALT_align;
		uint16_t Alt_Scale_en;
		uint16_t Alt_Scale_panle;
		uint16_t Alt_Scale_posX;
		uint16_t Alt_Scale_align;
		uint16_t Alt_Scale_source;
		
		uint16_t TSPD_en;
		uint16_t TSPD_panel;
		uint16_t TSPD_posX;
		uint16_t TSPD_posY;
		uint16_t TSPD_fontsize;
		uint16_t TSPD_align;
		uint16_t Speed_scale_en;
		uint16_t Speed_scale_panel;
		uint16_t Speed_scale_posX;
		uint16_t Speed_scale_align;
		uint16_t Speed_scale_source;

		uint16_t Throt_en;
		uint16_t Throt_panel;
		uint16_t Throt_scale_en;
		uint16_t Throt_posX;
		uint16_t Throt_posY;
		
		uint16_t CWH_home_dist_en;
		uint16_t CWH_home_dist_panel;
		uint16_t CWH_home_dist_posX;
		uint16_t CWH_home_dist_posY;
		uint16_t CWH_home_dist_fontsize;
		uint16_t CWH_home_dist_align;
		
		uint16_t CWH_wp_dist_en;
		uint16_t CWH_wp_dist_panel;
		uint16_t CWH_wp_dist_posX;
		uint16_t CWH_wp_dist_posY;
		uint16_t CWH_wp_dist_fontsize;
		uint16_t CWH_wp_dist_align;
		
		uint16_t CWH_Tmode_en;
		uint16_t CWH_Tmode_panel;
		uint16_t CWH_Tmode_posY;
		uint16_t CWH_Nmode_en;
		uint16_t CWH_Nmode_panel;
		uint16_t CWH_Nmode_posX;
		uint16_t CWH_Nmode_posY;
		uint16_t CWH_Nmode_radius;
		uint16_t CWH_Nmode_home_radius;
		uint16_t CWH_Nmode_wp_radius;
		
		uint16_t Atti_mp_en;
		uint16_t Atti_mp_panel;
		uint16_t Atti_mp_mode;
		uint16_t Atti_3D_en;
		uint16_t Atti_3D_panel;
		
		//misc
		uint16_t Units_mode;
		uint16_t Max_panels;
		
		uint16_t PWM_Video_en;
		uint16_t PWM_Video_ch;
		uint16_t PWM_Video_value;
		uint16_t PWM_Panel_en;
		uint16_t PWM_Panel_ch;
		uint16_t PWM_Panel_value;
		
		uint16_t Alarm_posX;
		uint16_t Alarm_posY;
		uint16_t Alarm_fontsize;
		uint16_t Alarm_align;
		uint16_t Alarm_GPS_status_en;
		uint16_t Alarm_low_batt_en;
		uint16_t Alarm_low_batt;
		uint16_t Alarm_low_speed_en;
		uint16_t Alarm_low_speed;
		uint16_t Alarm_over_speed_en;
		uint16_t Alarm_over_speed;
		uint16_t Alarm_low_alt_en;
		uint16_t Alarm_low_alt;
		uint16_t Alarm_over_alt_en;
		uint16_t Alarm_over_alt;
		
		uint16_t ClimbRate_en;
		uint16_t ClimbRate_panel;
		uint16_t ClimbRate_posX;
		uint16_t ClimbRate_posY;
		uint16_t ClimbRate_fontsize;
		//uint16_t ClimbRate_align;
		
		uint16_t RSSI_en;
		uint16_t RSSI_panel;
		uint16_t RSSI_posX;
		uint16_t RSSI_posY;
		uint16_t RSSI_fontsize;
		uint16_t RSSI_align;
		uint16_t RSSI_min;
		uint16_t RSSI_max;
		uint16_t RSSI_raw_en;
		
		uint16_t FC_Protocol;
		
        uint16_t Wind_en;
        uint16_t Wind_panel;
        uint16_t Wind_posX;
        uint16_t Wind_posY;

        uint16_t Time_type;

        uint16_t Throttle_Scale_Type;

        uint16_t Atti_mp_posX;
        uint16_t Atti_mp_posY;
        uint16_t Atti_mp_scale_real;
        uint16_t Atti_mp_scale_frac;
        uint16_t Atti_3D_posX;
        uint16_t Atti_3D_posY;
        uint16_t Atti_3D_scale_real;
        uint16_t Atti_3D_scale_frac;
        uint16_t Atti_3D_map_radius;

        uint16_t osd_offsetY;
        uint16_t osd_offsetX;

        uint16_t firmware_ver;
        uint16_t video_mode;
//		//below is unused. if add a param, reduce one item here
//		uint16_t unused[EERROM_SIZE/2 - 104];
	}params;
} EEPROM_BUF_TYPE;

extern EEPROM_BUF_TYPE eeprom_buffer;
				
#endif	//__OSD_CONFIG_H
