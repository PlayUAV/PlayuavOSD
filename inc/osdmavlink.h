#ifndef __OSD_MAVLINK_H
#define __OSD_MAVLINK_H

// mavlink error - anonymous unions are only supported in --gun mode, 
// or when enable with #pragma anon_unions 
#pragma anon_unions

#include "board.h"
#include "usart2.h"
// we have separate helpers disabled to make it possible
// to select MAVLink 1.0 in the arduino GUI build
//#define MAVLINK_SEPARATE_HELPERS
#include "ardupilotmega/version.h"

// this allows us to make mavlink_message_t much smaller
#define MAVLINK_MAX_PAYLOAD_LEN MAVLINK_MAX_DIALECT_PAYLOAD_SIZE
#define MAVLINK_COMM_NUM_BUFFERS 1
#include "mavlink_types.h"

/// MAVLink system definition
extern mavlink_system_t mavlink_system;
/// Send a byte to the nominated MAVLink channel
///
/// @param chan		Channel to send to
/// @param ch		Byte to send
///
static inline void comm_send_ch(mavlink_channel_t chan, uint8_t ch)
{
    switch(chan) {
	case MAVLINK_COMM_0:
		mavlink_usart_send_byte(ch);
		break;
	default:
		break;
	}
}

#define MAVLINK_USE_CONVENIENCE_FUNCTIONS
#include "ardupilotmega/mavlink.h"

void request_mavlink_rates(void);
void request_mission_count(void);
void request_mission_item(uint16_t index);
    
void MavlinkTask(void *pvParameters);

#endif	//__OSD_MAVLINK_H
