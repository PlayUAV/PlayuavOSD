#ifndef __DJICANDECODER_H
#define __DJICANDECODER_H	 

#include "board.h" 

// Uncommnet to read smart battery data (if available) e.g. on Phantom controller
//#define GET_SMART_BATTERY_DATA

#define NAZA_MESSAGE_COUNT   3
#define NAZA_MESSAGE_NONE    0x0000
#define NAZA_MESSAGE_MSG1002 0x1002
#define NAZA_MESSAGE_MSG1003 0x1003
#define NAZA_MESSAGE_MSG1009 0x1009
#ifdef GET_SMART_BATTERY_DATA
#define NAZA_MESSAGE_MSG0926 0x0926
#endif

typedef enum { 
	NO_FIX1 = 0, 
	FIX_2D = 2, 
	FIX_3D = 3, 
	FIX_DGPS = 4 
} fixType_t;  // GPS fix type

typedef enum { 
	MANUAL = 0, 
	GPS = 1, 
	FAILSAFE = 2, 
	ATTI = 3 
} mode_t;  // Flying mode

typedef enum { 
	RC_UNUSED_1 = 0, 
	RC_A = 1, 
	RC_E = 2, 
	RC_R = 3, 
	RC_U = 4, 
	RC_T = 5, 
	RC_UNUSED_2 = 6, 
	RC_X1 = 7, 
	RC_X2 = 8, 
	RC_UNUSED_3 = 9 
} rcInChan_t;  // RC channel index

#ifdef GET_SMART_BATTERY_DATA
    typedef enum { CELL_1 = 0, CELL_2 = 1, CELL_3 = 2, } smartBatteryCell_t;  // Smart battery cell index
#endif

typedef struct{
	uint16_t id;
	uint16_t len;
} naza_msg_header_t;


//typedef struct{
//	naza_msg_header_t header;
//	double  lon;           // longitude (radians)
//	double  lat;           // lattitude (radians)
//	float   altGps;        // altitude from GPS (meters)
//	float   accX;          // accelerometer X axis data (??)
//	float   accY;          // accelerometer Y axis data (??)
//	float   accZ;          // accelerometer Z axis data (??)
//	float   gyrX;          // gyroscope X axis data (??)
//	float   gyrY;          // gyroscope Y axis data (??)
//	float   gyrZ;          // gyroscope Z axis data (??)
//	float   altBaro;       // altitude from barometric sensor (meters)
//	float   unk0[7];
//	float   northVelocity; // averaged northward velocity or 0 when less than 5 satellites locked (m/s)
//	float   eastVelocity;  // averaged eastward velocity or 0 when less than 5 satellites locked (m/s)
//	float   downVelocity;  // downward velocity (barometric) (m/s)
//	float   unk1[3];
//	int16_t  magCalX;       // calibrated magnetometer X axis data
//	int16_t  magCalY;       // calibrated magnetometer Y axis data
//	int16_t  magCalZ;       // calibrated magnetometer Y axis data
//	uint8_t  unk2[10];
//	uint8_t  numSat;        // number of locked satellites
//	uint8_t  unk3;
//	uint16_t seqNum;        // sequence number - increases with every message
//} naza_msg1002_t;

////GPS data
//typedef struct {
//	naza_msg_header_t header;
//	uint32_t dateTime;      // date/time
//	uint32_t lon;           // longitude (x10^7, degree decimal)
//	uint32_t lat;           // lattitude (x10^7, degree decimal)
//	uint32_t altGps;        // altitude from GPS (millimeters)
//	uint32_t hae;           // horizontal accuracy estimate (millimeters)
//	uint32_t vae;           // vertical accuracy estimate (millimeters)
//	uint8_t  unk0[4];
//	int32_t  northVelocity; // northward velocity (cm/s)
//	int32_t  eastVelocity;  // eastward velocity (cm/s)
//	int32_t  downVelocity;  // downward velocity (cm/s)
//	uint16_t pdop;          // position DOP (x100)
//	uint16_t vdop;          // vertical DOP (see uBlox NAV-DOP message for details)
//	uint16_t ndop;          // northing DOP (see uBlox NAV-DOP message for details)
//	uint16_t edop;          // easting DOP (see uBlox NAV-DOP message for details)
//	uint8_t  numSat;        // number of locked satellites
//	uint8_t  unk1;
//	uint8_t  fixType;       // fix type (0 - no lock, 2 - 2D lock, 3 - 3D lock, not sure if other values can be expected - see uBlox NAV-SOL message for details)
//	uint8_t  unk2;
//	uint8_t  fixStatus;     // fix status flags (see uBlox NAV-SOL message for details)
//	uint8_t  unk3[3];
//	uint16_t seqNum;        // sequence number - increases with every message
//} naza_msg1003_t;

//// input/output message
//typedef struct{
//	naza_msg_header_t header;
//	uint8_t  unk1[4];
//	uint16_t motorOut[8];  // motor output (M1/M2/M3/M4/M5/M6/F1/F2)
//	uint8_t  unk2[4];
//	int16_t  rcIn[10];     // RC controller input (order: unused/A/E/R/U/T/unused/X1/X2/unused)
//	uint8_t  unk3[11];
//	uint8_t  flightMode;   // (0 - manual, 1 - GPS, 2 - failsafe, 3 - atti)
//	uint8_t  unk4[8];
//	double   homeLat;      // home lattitude (radians)
//	double   homeLon;      // home longitude (radians)
//	float    homeAltBaro;  // home altitude from barometric sensor plus 20m (meters)
//	uint16_t seqNum;       // sequence number - increases with every message
//	uint8_t  unk5[2];
//	float    stabRollIn;   // attitude stabilizer roll input (-1000~1000)
//	float    stabPitchIn;  // attitude stabilizer pitch input (-1000~1000)
//	float    stabThroIn;   // altitude stabilizer throttle input (-1000~1000)
//	//uint8_t  unk6[4];
//	float    actAileIn;    // actual aileron input, mode and arm state dependent (-1000~1000)
//	float    actElevIn;    // actual elevator input, mode and arm state dependent (-1000~1000)
//	float    actThroIn;    // actual throttle input, mode and arm state dependent (-1000~1000)
//	uint16_t batVolt;      // main battery voltage (milivolts)
//	uint16_t becVolt;      // BEC voltage (milivolts)
//	uint8_t  unk7[8];
//	uint8_t  controlMode;  // (0 - GPS/failsafe, 1 - waypoint mode?, 3 - manual, 6 - atti)
//	uint8_t  unk8[5];
//	int16_t  gyrScalX;     // ???
//	int16_t  gyrScalY;     // ???
//	int16_t  gyrScalZ;     // ???
//	//uint8_t  unk9[32];
//	uint8_t  unk9[28];
//	float    downVelocity; // downward velocity (m/s)
//	float    altBaro;      // altitude from barometric sensor (meters)
//	float    roll;         // roll angle (radians)
//	float    pitch;        // pitch angle (radians)
//} naza_msg1009_t;


typedef struct{
	naza_msg_header_t header;

	uint8_t unk43[20];
	
	float   accX;          // accelerometer X axis data - OK
	float   accY;          // accelerometer Y axis data - OK
	float   accZ;          // accelerometer Z axis data - OK
	
	float   unk12[3];
	
	float   altBaro;       // altitude from barometric sensor (meters) - OK
	
	float   unk11;
	float   headCompX;     // compensated heading X component
	float   unk0[2];
	float   headCompY;     // compensated heading Y component
	float   unk1[3];
	float   northVelocity; // averaged northward velocity or 0 when less than 5 satellites locked (m/s)
	float   eastVelocity;  // averaged eastward velocity or 0 when less than 5 satellites locked (m/s)
	float   downVelocity;  // downward velocity (barometric) (m/s)
	float   unk2[3];
	int16_t  magCalX;       // calibrated magnetometer X axis data
	int16_t  magCalY;       // calibrated magnetometer Y axis data
	int16_t  magCalZ;       // calibrated magnetometer Y axis data
	uint8_t  unk3[6];
	uint8_t  numSat;        // number of locked satellites
	uint8_t  unk4[5];
	uint16_t seqNum;        // sequence number - increases with every message

} naza_msg1002_t;

//GPS data
typedef struct {
	naza_msg_header_t header;
	uint32_t dateTime;      // date/time
	uint32_t lon;           // longitude (x10^7, degree decimal) - OK
	uint32_t lat;           // lattitude (x10^7, degree decimal) - OK
	uint32_t altGps;        // altitude from GPS (millimeters)
	uint32_t hae;           // horizontal accuracy estimate (millimeters)
	uint32_t vae;           // vertical accuracy estimate (millimeters)
	uint8_t  unk0[4];
	int32_t  northVelocity; // northward velocity (cm/s) - OK
	int32_t  eastVelocity;  // eastward velocity (cm/s) - OK
	int32_t  downVelocity;  // downward velocity (cm/s) - OK
	uint16_t pdop;          // position DOP (x100)
	uint16_t vdop;          // vertical DOP (see uBlox NAV-DOP message for details) - OK
	uint16_t ndop;          // northing DOP (see uBlox NAV-DOP message for details) - OK
	uint16_t edop;          // easting DOP (see uBlox NAV-DOP message for details) - OK
	uint8_t  numSat;        // number of locked satellites - OK
	uint8_t  unk1;
	uint8_t  fixType;       // - OK fix type (0 - no lock, 2 - 2D lock, 3 - 3D lock, not sure if other values can be expected - see uBlox NAV-SOL message for details)
	uint8_t  unk2;
	uint8_t  fixStatus;     //  - OK fix status flags (see uBlox NAV-SOL message for details)
	uint8_t  unk3[3];
	uint16_t seqNum;        // sequence number - increases with every message
} naza_msg1003_t;

// input/output message
typedef struct{
	naza_msg_header_t header;
	uint8_t  unk1[4];
	uint16_t motorOut[8];  // motor output (M1/M2/M3/M4/M5/M6/F1/F2) 20
	uint8_t  unk2[4];
	int16_t  rcIn[10];     // RC controller input (order: unused/A/E/R/U/T/unused/X1/X2/unused) 44
	uint8_t  unk3[11];     // 55
	uint8_t  flightMode;   // (0 - manual, 1 - GPS, 2 - failsafe, 3 - atti)56
	uint8_t  unk4[8];      //64
	double   homeLat;      // home lattitude (radians)72
	double   homeLon;      // home longitude (radians) 80
	float    homeAltBaro;  // home altitude from barometric sensor plus 20m (meters)
	uint16_t seqNum;       // sequence number - increases with every message
	uint8_t  unk5[2];
	float    stabRollIn;   // attitude stabilizer roll input (-1000~1000)
	float    stabPitchIn;  // attitude stabilizer pitch input (-1000~1000)
	float    stabThroIn;   // altitude stabilizer throttle input (-1000~1000) 100
	//uint8_t  unk6[4];
	float    actAileIn;    // actual aileron input, mode and arm state dependent (-1000~1000)
	float    actElevIn;    // actual elevator input, mode and arm state dependent (-1000~1000)
	float    actThroIn;    // actual throttle input, mode and arm state dependent (-1000~1000)112
	uint16_t batVolt;      // main battery voltage (milivolts)
	uint16_t becVolt;      // BEC voltage (milivolts)
	uint8_t  unk7[8];
	uint8_t  controlMode;  // (0 - GPS/failsafe, 1 - waypoint mode?, 3 - manual, 6 - atti)125
	uint8_t  unk8[5];
	int16_t  gyrScalX;     // ???
	int16_t  gyrScalY;     // ???
	int16_t  gyrScalZ;     // ???136
	uint8_t  unk9[28];				//164
	float    downVelocity; // downward velocity (m/s)168
	float    altBaro;      // altitude from barometric sensor (meters)172
	float    roll;         // roll angle (radians)176
	float    pitch;        // pitch angle (radians)180
	float  unk10;
} naza_msg1009_t;

#ifdef GET_SMART_BATTERY_DATA
	typedef struct{
		naza_msg_header_t header;
		uint16_t designCapacity;  // design capacity in mAh
		uint16_t fullCapacity;    // design capacity in mAh
		uint16_t currentCapacity; // design capacity in mAh
		uint16_t voltage;         // battry voltage in mV
		int16_t  current;         // current in mA
		uint8_t  lifePercent;     // percentage of life
		uint8_t  chargePercent;   // percentage of charge
		int16_t temperature;      // temperature in tenths of a degree Celsius
		uint16_t dischargeCount;  // number of discharges
		uint16_t serialNumber;    // serial number
		uint16_t cellVoltage[3];  // individual cell voltage in mV
		uint8_t  unk1[11];
	} naza_msg0926_t;
#endif

typedef union{
	uint8_t           bytes[256]; // Max message size (184) + header size (4) + footer size (4)
	naza_msg_header_t header;
	naza_msg1002_t    msg1002;
	naza_msg1003_t    msg1003;
	naza_msg1009_t    msg1009;
#ifdef GET_SMART_BATTERY_DATA
	naza_msg0926_t    msg0926;
#endif
} naza_msg_t;
	


void DJICanTask(void *pvParameters);


// Start the NazaCanDecoder
void 		DJI_CAN_Decoder_init(void);

// Decode incoming CAN message if any (shall be called in a loop)	
uint16_t 	DJI_CAN_Decoder_decode(void);      

// Periodically (every 2 sec., keeps it inner counter) sends a heartbeat message to the controller	
void   		DJI_CAN_Decoder_heartbeat(void);     
	
u8 can_recv_msg(CanRxMsg* canmsg);
u8 can_send_msg(CanTxMsg* cantxmsg);

void djican_test_usart_init(uint32_t baudRate);

	
#endif	//__DJICANDECODER_H
