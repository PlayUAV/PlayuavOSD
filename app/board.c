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
#include "board.h"
#include "led.h"
#include "osdproc.h"
#include "osdcore.h"
#include "osdmavlink.h"
#include "dsptest.h"
#include "max7456.h"
#include "DJICanDecoder.h"
#include "usart2.h"
#include "osdconfig.h"
#include "tm_stm32f4_usb_vcp.h"
#include "osdconfig.h"
#include "math3d.h"
#include "osdvar.h"
#include "uavTalk.h"

void vTaskHeartBeat(void *pvParameters);
void vTask10HZ(void *pvParameters);
void triggerVideo(void);
void triggerPanel(void);

uint8_t video_switch=0;

xTaskHandle xTaskVCPHandle;

int32_t pwmPanelNormal = 0;
void board_init(void)
{
	GPIO_InitTypeDef  gpio;
	SystemCoreClockUpdate( );
	
	// turn on peripherals needed by all
	RCC_AHB1PeriphClockCmd(	RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | 
													RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD |
													RCC_AHB1Periph_DMA1  | RCC_AHB1Periph_DMA2  |
													RCC_AHB1Periph_BKPSRAM, ENABLE);
	
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_SPI1 | RCC_APB2Periph_TIM1 | RCC_APB2Periph_SYSCFG, ENABLE);
	
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2 | RCC_APB1Periph_SPI3 | RCC_APB1Periph_TIM2 | 
													RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4 | RCC_APB1Periph_PWR, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
	RCC_LSEConfig(RCC_LSE_OFF);
	
	//This is a trick to perform a USB re-enumerate
	gpio.GPIO_Pin = GPIO_Pin_12;
	gpio.GPIO_Speed = GPIO_Speed_100MHz;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &gpio);  
	GPIO_SetBits(GPIOB,GPIO_Pin_12);
	Delay_us(300000);
	GPIO_ResetBits(GPIOB,GPIO_Pin_12);
	
	STM_EVAL_LEDInit(LED_BLUE);
	STM_EVAL_LEDInit(LED_GREEN);
	
	gpio.GPIO_Pin = GPIO_Pin_0;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	gpio.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOC, &gpio);
	GPIO_SetBits(GPIOC, GPIO_Pin_0);
	gpio.GPIO_Pin = GPIO_Pin_1;
	gpio.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOC, &gpio);
	GPIO_ResetBits(GPIOC, GPIO_Pin_1);

	//SPI1 output to electronic switch to control mask
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_6; // SPI1 MISO
	gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &gpio);
	GPIO_ResetBits(GPIOA, GPIO_Pin_6);

	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);
		
	// max7456 SPI MOIS MISO SLK pin config
	gpio.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_PuPd  = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOC, &gpio);
	
	// max7456 SPI CS Pin
	gpio.GPIO_Pin = GPIO_Pin_15;
	gpio.GPIO_Mode = GPIO_Mode_OUT;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &gpio);
	GPIO_SetBits(GPIOA,GPIO_Pin_15);
	
	SPI_MAX7456_init();

	LoadParams();
	
	Build_Sin_Cos_Tables();
	
	/* Initialize USB VCP */    
	TM_USB_VCP_Init();
}

void module_init(void)
{
	xTaskCreate( vTaskHeartBeat, (const char*)"Task Heartbeat", 
		STACK_SIZE_MIN, NULL, THREAD_PRIO_LOW, NULL );
	
	xTaskCreate( vTask10HZ, (const char*)"Task 10HZ", 
		STACK_SIZE_MIN, NULL, THREAD_PRIO_NORMAL, NULL );
	
	xTaskCreate( vTaskOSD, (const char*)"Task OSD", 
		STACK_SIZE_MIN*2, NULL, THREAD_PRIO_HIGHEST, NULL );
	
	xTaskCreate( vTaskVCP, (const char*)"Task VCP", 
	STACK_SIZE_MIN*2, NULL, THREAD_PRIO_HIGH, &xTaskVCPHandle );
	
	switch(eeprom_buffer.params.FC_Type){
		case FC_APM_PIXHAWK:
			xTaskCreate( MavlinkTask, (const char*)"Task Mavlink", 
						 STACK_SIZE_MIN*2, NULL, THREAD_PRIO_HIGH, NULL );
			break;
		case FC_CC3D_REVO:
			xTaskCreate( UAVTalkTask, (const char*)"Task UAVTalk", 
						 STACK_SIZE_MIN*2, NULL, THREAD_PRIO_HIGH, NULL );
			break;
		default:
			break;
	}
	

//	xTaskCreate( DJICanTask, (const char*)"DJI CAN", 
//	STACK_SIZE_MIN, NULL, THREAD_PRIO_HIGH, NULL );
}

void vTaskHeartBeat(void *pvParameters)
{
	for(;;)
	{
		STM_EVAL_LEDToggle(LED_GREEN);
		vTaskDelay( 500 / portTICK_RATE_MS );
	}
}

void vTask10HZ(void *pvParameters)
{
	for(;;)
	{
		vTaskDelay( 100 / portTICK_RATE_MS );
		
		//trigger video switch
		if(eeprom_buffer.params.PWM_Video_en)
		{
			triggerVideo();
		}
		
		//trigger panel switch
		if(eeprom_buffer.params.PWM_Panel_en)
		{
			triggerPanel();
		}
		
		//if no mavlink update for 2 secs, show waring and request mavlink rate again
		if(GetSystimeMS() > (lastMAVBeat + 2200))
		{
			heatbeat_start_time = 0;
			waitingMAVBeats = 1;
		}

		if(enable_mav_request == 1)
		{
			for(int n = 0; n < 3; n++){
				request_mavlink_rates();//Three times to certify it will be readed
				vTaskDelay(50/portTICK_RATE_MS);
			}
			enable_mav_request = 0;
			waitingMAVBeats = 0;
			lastMAVBeat = GetSystimeMS();
		}
	}
}

void triggerVideo(void)
{
	static uint16_t video_ch_raw;
	static bool video_trigger = false;
	
	video_ch_raw = 0;
	if(eeprom_buffer.params.PWM_Video_ch == 5) video_ch_raw = osd_chan5_raw;
	else if(eeprom_buffer.params.PWM_Video_ch == 6) video_ch_raw = osd_chan6_raw;
	else if(eeprom_buffer.params.PWM_Video_ch == 7) video_ch_raw = osd_chan7_raw;
	else if(eeprom_buffer.params.PWM_Video_ch == 8) video_ch_raw = osd_chan8_raw;

	if((video_ch_raw > eeprom_buffer.params.PWM_Video_value))
	{
		if(!video_trigger)
		{
			video_trigger = true;
			if(video_switch == 0)
			{
				video_switch = 1;
				GPIO_ResetBits(GPIOC, GPIO_Pin_0);
				GPIO_SetBits(GPIOC, GPIO_Pin_1);
			}
			else
			{
				video_switch = 0;
				GPIO_SetBits(GPIOC, GPIO_Pin_0);
				GPIO_ResetBits(GPIOC, GPIO_Pin_1);
			}
		}
	}
	else
	{
		video_trigger = false;
	}
}

void triggerPanel(void)
{
	static uint16_t panel_ch_raw;
	static bool panel_trigger = false;
	
	panel_ch_raw = 0;
	if(eeprom_buffer.params.PWM_Panel_ch == 5) panel_ch_raw = osd_chan5_raw;
	else if(eeprom_buffer.params.PWM_Panel_ch == 6) panel_ch_raw = osd_chan6_raw;
	else if(eeprom_buffer.params.PWM_Panel_ch == 7) panel_ch_raw = osd_chan7_raw;
	else if(eeprom_buffer.params.PWM_Panel_ch == 8) panel_ch_raw = osd_chan8_raw;

	if((panel_ch_raw > eeprom_buffer.params.PWM_Panel_value))
	{
		if(!panel_trigger)
		{
			panel_trigger = true;
			current_panel++;
		}
	}
	else
	{
		panel_trigger = false;
	}
}

/**
 *
 * @brief   Returns the current system time.
 *
 * @returns current system time
 *
 */
uint32_t GetSystimeMS(void)
{
	return (uint32_t)TICKS2MS(xTaskGetTickCount());
}

float Rad2Deg(float x)
{
    return x * (180.0F / M_PI);
}

float Deg2Rad(float x)
{
    return x * (M_PI / 180.0F);
}

void Delay_us(u32 nus)
{		
	u16 i=0;
	while(nus--)
	{
		i=12;
		while(i--);
	}
}
