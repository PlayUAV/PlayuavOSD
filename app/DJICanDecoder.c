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
 
#include "DJICanDecoder.h" 
#include "osdvar.h"
#include "usart2.h"
#include <stdio.h>

/*
 * DJI NAZA/Phantom/A2 CAN bus communication protocol 
 * http://www.rcgroups.com/forums/showthread.php?t=2071772
 */
uint32_t heartbeatTime = 0;
CanRxMsg canRcvMsg;

CanTxMsg HEARTBEAT_1 = {0x108, 0, CAN_Id_Standard, CAN_RTR_Data, 8, {0x55, 0xAA, 0x55, 0xAA, 0x07, 0x10, 0x00, 0x00}};
CanTxMsg HEARTBEAT_2 = {0x108, 0, CAN_Id_Standard, CAN_RTR_Data, 4, {0x66, 0xCC, 0x66, 0xCC}};
//static const CAN_filter_t FILTER_MASK;
//static const CAN_filter_t FILTER_090;
//static const CAN_filter_t FILTER_108;
//static const CAN_filter_t FILTER_7F8;
uint8_t canMsgIdIdx;
uint8_t canMsgByte = 0;

naza_msg_t msgBuf[NAZA_MESSAGE_COUNT];
uint16_t msgLen[NAZA_MESSAGE_COUNT];
uint16_t msgIdx[NAZA_MESSAGE_COUNT];
uint8_t header[NAZA_MESSAGE_COUNT];
uint8_t footer[NAZA_MESSAGE_COUNT];
uint8_t collectData[NAZA_MESSAGE_COUNT];

double test1;       
double test2;       
double test3;       
double test4;  

double lon;       // longitude in degree decimal
double lat;       // latitude in degree decimal
float alt;       // altitude in m (from barometric sensor)
float altBaroGround;
double gpsAlt;    // altitude in m (from GPS)
double spd;       // speed in m/s
fixType_t fix;     // fix type (see fixType_t enum)
uint8_t sat;       // number of satellites 
double heading;   // heading in degrees (titlt compensated)
double headingNc; // heading in degrees (not titlt compensated)
double cog;       // course over ground
double vsi;       // vertical speed indicator (barometric) in m/s (a.k.a. climb speed)
double hdop;      // horizontal dilution of precision
double vdop;      // vertical dilution of precision
double gpsVsi;    // vertical speed indicator (GPS based) in m/s (a.k.a. climb speed)
float pitchRad;   // pitch in radians 
float rollRad;    // roll in radians
float pitch;      // pitch in degrees 
float roll;       // roll in degrees 
uint8_t year;      // year (minus 2000)
uint8_t month;
uint8_t day;
uint8_t hour;    // hour (for time between 16:00 and 23:59 the hour returned from GPS module is actually 00:00 - 7:59)
uint8_t minute;
uint8_t second;
uint16_t battery; // battery voltage in mV
int16_t rcIn[10]; // RC stick input (-1000~1000), use rcInChan_t enum to index the table
mode_t mode;      // flight mode (see mode_t enum)
double homelat;
double homelon;


const	int numReadings = 20;
float	readings[numReadings];	
float	total=0;
float	headingNorm =0;		//Normalised heading based on 5 sample point average, every 0.5sec
int		counter=0;

#ifdef GET_SMART_BATTERY_DATA
    uint8_t  batteryPercent; // smart battery charge percentage (0-100%)
    uint16_t batteryCell[3]; // smart battery cell voltage in mV, use smartBatteryCell_t enum to index the table
#endif



void DJICanTask(void *pvParameters)
{
	uint32_t messageId;
	uint32_t currTime, attiTime, otherTime, clockTime;
//    debug_usart_init(115200);
	DJI_CAN_Decoder_init();
	
	currTime = 0;
	attiTime = 0;
	otherTime = 0;
	clockTime = 0;
	altBaroGround = 0.0;
	for (int i = 0; i < numReadings; i++)
		readings[i] = 0;
	
    while (1) 
	{
		messageId = DJI_CAN_Decoder_decode();
		currTime = GetSystimeMS();
		if(attiTime < currTime)
		{
			attiTime = currTime + 100;
			//SERIAL_PORT.print("Pitch: "); SERIAL_PORT.print(NazaCanDecoder.getPitch());
			//SERIAL_PORT.print(", Roll: "); SERIAL_PORT.println(NazaCanDecoder.getRoll());
			//if((pitch != 0) || (roll != 0))
//			printf("Pitch: %i, Roll: %i\n", pitch, roll);
		}
		
		//
		osd_vbat_A = (float)battery/1000;
#ifdef GET_SMART_BATTERY_DATA		
		osd_battery_remaining_A = batteryPercent;
#endif		
		osd_pitch = pitch;
		osd_roll = roll;
		osd_groundspeed = spd;
		osd_downVelocity = vsi;
		
		osd_alt = alt;
		//osd_alt = gpsAlt;
		
		osd_mode = (uint8_t)mode;
		
		osd_satellites_visible = sat;
		osd_lat = lat;
		osd_lon = lon;
		osd_hdop = hdop;
		
		osd_home_lat = homelat;
		osd_home_lon = homelon;
		
//		total = total - readings[counter];
//		readings[counter]=heading;
//		total = total + readings[counter];
//		counter++;
//		if (counter >= numReadings ) counter = 0;
//		headingNorm = round(total/numReadings);
//		if(headingNorm < 0) headingNorm += 360.0;
//		
//		osd_heading = heading;
		DJI_CAN_Decoder_heartbeat();
    }
}

void DJI_CAN_Decoder_init()
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	CAN_InitTypeDef        CAN_InitStructure;
 	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
#if CAN_RX0_INT_ENABLE 
   	NVIC_InitTypeDef  NVIC_InitStructure;
#endif            											 

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_CAN1);	
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_CAN1);
	
	//CTX->PB9 CRX->PB8
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);	
	
	CAN_DeInit(CAN1);	
 	CAN_InitStructure.CAN_TTCM=DISABLE;	
 	CAN_InitStructure.CAN_ABOM=DISABLE;	
  	CAN_InitStructure.CAN_AWUM=DISABLE;	
  	CAN_InitStructure.CAN_NART=ENABLE;	
  	CAN_InitStructure.CAN_RFLM=DISABLE;	
  	CAN_InitStructure.CAN_TXFP=DISABLE;	
  	CAN_InitStructure.CAN_Mode= CAN_Mode_Normal;	
  	CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;	
  	CAN_InitStructure.CAN_BS1=CAN_BS1_2tq; 
  	CAN_InitStructure.CAN_BS2=CAN_BS2_4tq;
  	CAN_InitStructure.CAN_Prescaler=6;    
  	CAN_Init(CAN1, &CAN_InitStructure);

 	CAN_FilterInitStructure.CAN_FilterNumber=0;	
 	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;
 	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; 

  	CAN_FilterInit(&CAN_FilterInitStructure);
#if CAN_RX0_INT_ENABLE
	
	CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);		    
  
  	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;    
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
	
}
 
uint16_t DJI_CAN_Decoder_decode(void)
{
	uint16_t msgId = NAZA_MESSAGE_NONE;
	if(can_recv_msg(&canRcvMsg))
	{
		if(canRcvMsg.StdId == 0x090) canMsgIdIdx = 0;
		else if(canRcvMsg.StdId == 0x108) canMsgIdIdx = 1;
		else if(canRcvMsg.StdId == 0x7F8) canMsgIdIdx = 2;
		else return msgId; // we don't care about other CAN messages
		for(uint8_t i = 0; i < canRcvMsg.DLC; i++)
		{
			canMsgByte = canRcvMsg.Data[i];
			if(collectData[canMsgIdIdx] == 1)
			{
				msgBuf[canMsgIdIdx].bytes[msgIdx[canMsgIdIdx]] = canMsgByte;
				if(msgIdx[canMsgIdIdx] == 3)
				{
					msgLen[canMsgIdIdx] = msgBuf[canMsgIdIdx].header.len;
				}
				msgIdx[canMsgIdIdx] += 1;
				if((msgIdx[canMsgIdIdx] > (msgLen[canMsgIdIdx] + 8)) || (msgIdx[canMsgIdIdx] > 256)) 
					collectData[canMsgIdIdx] = 0;
			}
			
			// Look fo header
			if(canMsgByte == 0x55) 
			{ 
				if(header[canMsgIdIdx] == 0) 
					header[canMsgIdIdx] = 1; 
				else if(header[canMsgIdIdx] == 2) 
					header[canMsgIdIdx] = 3; 
				else header[canMsgIdIdx] = 0;
			}
			else if(canMsgByte == 0xAA) 
			{ 
				if(header[canMsgIdIdx] == 1) 
					header[canMsgIdIdx] = 2; 
				else if(header[canMsgIdIdx] == 3) 
				{ 
					header[canMsgIdIdx] = 0; 
					collectData[canMsgIdIdx] = 1; 
					msgIdx[canMsgIdIdx] = 0; 
				} 
				else header[canMsgIdIdx] = 0;
			}
			else 
			{
				header[canMsgIdIdx] = 0;
			}
	  
			// Look fo footer
			if(canMsgByte == 0x66) 
			{ 
				if(footer[canMsgIdIdx] == 0) 
					footer[canMsgIdIdx] = 1; 
				else if(footer[canMsgIdIdx] == 2) 
					footer[canMsgIdIdx] = 3; 
				else footer[canMsgIdIdx] = 0;
			}
			else if(canMsgByte == 0xCC) 
			{ 
				if(footer[canMsgIdIdx] == 1) 
					footer[canMsgIdIdx] = 2; 
				else if(footer[canMsgIdIdx] == 3) 
				{ 
					footer[canMsgIdIdx] = 0; 
					if(collectData[canMsgIdIdx] != 0) 
						collectData[canMsgIdIdx] = 2; 
				} 
				else footer[canMsgIdIdx] = 0;
			}
			else footer[canMsgIdIdx] = 0;
	  
			if(collectData[canMsgIdIdx] == 2)
			{
				if(msgIdx[canMsgIdIdx] == (msgLen[canMsgIdIdx] + 8))
				{
					if(msgBuf[canMsgIdIdx].header.id == NAZA_MESSAGE_MSG1002) 
					{
						//OSD data message
//						gyrx = msgBuf[canMsgIdIdx].msg1002.gyrX;
//						gyry = msgBuf[canMsgIdIdx].msg1002.gyrY;
//						gyrz = msgBuf[canMsgIdIdx].msg1002.gyrZ;
//						heading = atan2(gyry, gyrx) / M_PI * 180.0;
//						if(heading < 0) heading += 360.0;

//						test1 = (msgBuf[canMsgIdIdx].msg1002.lat)/ 10000000.0;
//						test3 = msgBuf[canMsgIdIdx].msg1002.altBaro;
//						test1 = sizeof(int16_t);
						
						float magCalX = msgBuf[canMsgIdIdx].msg1002.magCalX;
						float magCalY = msgBuf[canMsgIdIdx].msg1002.magCalY;
						headingNc = -atan2(magCalY, magCalX) / M_PI * 180.0;
						if(headingNc < 0) headingNc += 360.0;
						float headCompX = msgBuf[canMsgIdIdx].msg1002.headCompX;
						float headCompY = msgBuf[canMsgIdIdx].msg1002.headCompY;
						heading = atan2(headCompY, headCompX) / M_PI * 180.0;
						if(heading < 0) heading += 360.0;
						//sat = msgBuf[canMsgIdIdx].msg1002.numSat;
						//gpsAlt = msgBuf[canMsgIdIdx].msg1002.altGps;
//						lat = msgBuf[canMsgIdIdx].msg1002.lat / M_PI * 180.0;
//						lon = msgBuf[canMsgIdIdx].msg1002.lon / M_PI * 180.0;
						//alt = msgBuf[canMsgIdIdx].msg1002.altBaro;
						
						float nVel = msgBuf[canMsgIdIdx].msg1002.northVelocity;
						float eVel = msgBuf[canMsgIdIdx].msg1002.eastVelocity;
						spd = sqrt(nVel * nVel + eVel * eVel);
						cog = atan2(eVel, nVel) / M_PI * 180;
						if(cog < 0) cog += 360.0;
						vsi = -msgBuf[canMsgIdIdx].msg1002.downVelocity;
						msgId = NAZA_MESSAGE_MSG1002;

			
					} // NAZA_MESSAGE_MSG1002
					else if(msgBuf[canMsgIdIdx].header.id == NAZA_MESSAGE_MSG1003)
					{
						//GPS data
						lat = msgBuf[canMsgIdIdx].msg1003.lat;
						lon = msgBuf[canMsgIdIdx].msg1003.lon;
						test2 = (msgBuf[canMsgIdIdx].msg1003.lat)/ 10000000.0;
						//test4 = (msgBuf[canMsgIdIdx].msg1003.lon)/ 10000000.0;
						
						sat = msgBuf[canMsgIdIdx].msg1003.numSat;
						gpsVsi = -msgBuf[canMsgIdIdx].msg1003.downVelocity;
						vdop = (double)msgBuf[canMsgIdIdx].msg1003.vdop / 100;
						double ndop = (double)msgBuf[canMsgIdIdx].msg1003.ndop / 100; 
						double edop = (double)msgBuf[canMsgIdIdx].msg1003.edop / 100;
						hdop = sqrt(ndop * ndop + edop * edop);
						uint8_t fixType = msgBuf[canMsgIdIdx].msg1003.fixType;
						uint8_t fixFlags = msgBuf[canMsgIdIdx].msg1003.fixStatus;
						switch(fixType)
						{
						  case 2 : fix = FIX_2D; break;
						  case 3 : fix = FIX_3D; break;
						  default: fix = NO_FIX1; break;
						}
						if((fix != NO_FIX1) && (fixFlags & 0x02)) fix = FIX_DGPS;
						msgId = NAZA_MESSAGE_MSG1003;
					}// NAZA_MESSAGE_MSG1003
					else if(msgBuf[canMsgIdIdx].header.id == NAZA_MESSAGE_MSG1009)
					{
						test4 = (msgBuf[canMsgIdIdx].msg1009.altBaro);
						// input/output message
						for(uint8_t j = 0; j < 10; j++)
						{
							rcIn[j] = msgBuf[canMsgIdIdx].msg1009.rcIn[j];
						}
#ifndef GET_SMART_BATTERY_DATA
						battery = msgBuf[canMsgIdIdx].msg1009.batVolt;
#endif
						homelat = msgBuf[canMsgIdIdx].msg1009.homeLat;
						homelon = msgBuf[canMsgIdIdx].msg1009.homeLon;
						rollRad = msgBuf[canMsgIdIdx].msg1009.roll;
						pitchRad = msgBuf[canMsgIdIdx].msg1009.pitch;
						roll = (rollRad * 180.0 / M_PI);
						pitch = (pitchRad * 180.0 / M_PI);
						mode = (mode_t)msgBuf[canMsgIdIdx].msg1009.flightMode;	
						float tmpalt = msgBuf[canMsgIdIdx].msg1009.altBaro;
						if((tmpalt != 0.0) && (altBaroGround == 0.0))
							altBaroGround = tmpalt;
						//altBaroGround = msgBuf[canMsgIdIdx].msg1009.homeAltBaro;
						alt = msgBuf[canMsgIdIdx].msg1009.altBaro - altBaroGround;						
						msgId = NAZA_MESSAGE_MSG1009;
							
					} //NAZA_MESSAGE_MSG1009
#ifdef GET_SMART_BATTERY_DATA
					else if(msgBuf[canMsgIdIdx].header.id == NAZA_MESSAGE_MSG0926)
					{
						battery = msgBuf[canMsgIdIdx].msg0926.voltage;
						batteryPercent = msgBuf[canMsgIdIdx].msg0926.chargePercent;
						for(uint8_t j = 0; j < 3; j++){
							batteryCell[j] = msgBuf[canMsgIdIdx].msg0926.cellVoltage[j];
						}
						msgId = NAZA_MESSAGE_MSG0926;
					}
#endif
				} //if(msgIdx[canMsgIdIdx] == (msgLen[canMsgIdIdx] + 8))
				collectData[canMsgIdIdx] = 0;
			} // if(collectData[canMsgIdIdx] == 2)
		} //traverse the recv data
	} // if we receive msg
}

void DJI_CAN_Decoder_heartbeat(void)
{
	uint32_t currentTime = GetSystimeMS();
	if(currentTime > heartbeatTime)
	{
		heartbeatTime = currentTime + 2000;  // 0.5Hz so every 2000 milliseconds
		//CAN.write(HEARTBEAT_1);
		//CAN.write(HEARTBEAT_2);
		can_send_msg(&HEARTBEAT_1);
		can_send_msg(&HEARTBEAT_2);
	}
}
	
u8 can_recv_msg(CanRxMsg* canrxmsg)
{
	if( CAN_MessagePending(CAN1,CAN_FIFO0)==0)
		return false;
	
	CAN_Receive(CAN1, CAN_FIFO0, canrxmsg);
	return canrxmsg->DLC;
}

u8 can_send_msg(CanTxMsg* cantxmsg)
{	
	u8 mbox;
	u16 i=0;     
	mbox= CAN_Transmit(CAN1, cantxmsg);   
	i=0;
	while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	
	if(i>=0XFFF)return 1;
	return 0;		

}









