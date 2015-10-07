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
#include "uavtalk.h"
#include "osdproc.h"
#include "osdcore.h"
#include "osdmavlink.h"
#include "max7456.h"
#include "usart2.h"
#include "osdconfig.h"
#include "math3d.h"
#include "osdvar.h"

void vTaskHeartBeat(void *pvParameters);
void vTask10HZ(void *pvParameters);
void triggerVideo(void);
void triggerPanel(void);
void checkDefaultParam(void);

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
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_TIM1 | RCC_APB2Periph_SYSCFG, ENABLE);
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2 | RCC_APB1Periph_SPI3 | RCC_APB1Periph_TIM2 |
							RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4 | RCC_APB1Periph_TIM5 |  RCC_APB1Periph_TIM12 |
                     RCC_APB1Periph_PWR, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
	RCC_LSEConfig(RCC_LSE_OFF);

	LEDInit(LED_BLUE);
	LEDInit(LED_GREEN);

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

    Build_Sin_Cos_Tables();

    LoadParams();
    checkDefaultParam();

    SPI_MAX7456_init();

    atti_mp_scale = (float)eeprom_buffer.params.Atti_mp_scale_real + (float)eeprom_buffer.params.Atti_mp_scale_frac * 0.01;
    atti_3d_scale = (float)eeprom_buffer.params.Atti_3D_scale_real + (float)eeprom_buffer.params.Atti_3D_scale_frac * 0.01;
    atti_3d_min_clipX = eeprom_buffer.params.Atti_mp_posX - (uint32_t)(22*atti_mp_scale);
    atti_3d_max_clipX = eeprom_buffer.params.Atti_mp_posX + (uint32_t)(22*atti_mp_scale);
    atti_3d_min_clipY = eeprom_buffer.params.Atti_mp_posY - (uint32_t)(30*atti_mp_scale);
    atti_3d_max_clipY = eeprom_buffer.params.Atti_mp_posY + (uint32_t)(34*atti_mp_scale);
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
	STACK_SIZE_MIN*2, NULL, THREAD_PRIO_NORMAL, &xTaskVCPHandle );

	switch(eeprom_buffer.params.FC_Protocol){
		case PROTOCOL_MAVLINK:
			xTaskCreate( MavlinkTask, (const char*)"Task Mavlink",
						 STACK_SIZE_MIN*2, NULL, THREAD_PRIO_HIGH, NULL );
			break;
		case PROTOCOL_UAVTALK:
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
		LEDToggle(LED_GREEN);
		vTaskDelay( 500 / portTICK_RATE_MS );
	}
}

void vTask10HZ(void *pvParameters)
{
    for(;;)
        {
            vTaskDelay( 100 / portTICK_RATE_MS );

            // calculate osd_curr_consumed_mah(simulation)
            osd_curr_consumed_mah += (osd_curr_A * 0.00027777778f);

            // calculate osd_total_trip_dist(simulation)
            if (osd_groundspeed > 1.0f) osd_total_trip_dist += (osd_groundspeed * 0.1f);

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
            
            if(enable_mission_count_request == 1)
            {
                request_mission_count();
                enable_mission_count_request = 0;
            }

            if(enable_mission_item_request == 1)
            {
                request_mission_item(current_mission_item_req_index);
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
	else if(eeprom_buffer.params.PWM_Video_ch == 9) video_ch_raw = osd_chan9_raw;
	else if(eeprom_buffer.params.PWM_Video_ch == 10) video_ch_raw = osd_chan10_raw;
	else if(eeprom_buffer.params.PWM_Video_ch == 11) video_ch_raw = osd_chan11_raw;
	else if(eeprom_buffer.params.PWM_Video_ch == 12) video_ch_raw = osd_chan12_raw;
	else if(eeprom_buffer.params.PWM_Video_ch == 13) video_ch_raw = osd_chan13_raw;
	else if(eeprom_buffer.params.PWM_Video_ch == 14) video_ch_raw = osd_chan14_raw;
	else if(eeprom_buffer.params.PWM_Video_ch == 15) video_ch_raw = osd_chan15_raw;
	else if(eeprom_buffer.params.PWM_Video_ch == 16) video_ch_raw = osd_chan16_raw;

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
	else if(eeprom_buffer.params.PWM_Panel_ch == 9) panel_ch_raw = osd_chan9_raw;
    else if(eeprom_buffer.params.PWM_Panel_ch == 10) panel_ch_raw = osd_chan10_raw;
    else if(eeprom_buffer.params.PWM_Panel_ch == 11) panel_ch_raw = osd_chan11_raw;
    else if(eeprom_buffer.params.PWM_Panel_ch == 12) panel_ch_raw = osd_chan12_raw;
    else if(eeprom_buffer.params.PWM_Panel_ch == 13) panel_ch_raw = osd_chan13_raw;
    else if(eeprom_buffer.params.PWM_Panel_ch == 14) panel_ch_raw = osd_chan14_raw;
    else if(eeprom_buffer.params.PWM_Panel_ch == 15) panel_ch_raw = osd_chan15_raw;
    else if(eeprom_buffer.params.PWM_Panel_ch == 16) panel_ch_raw = osd_chan16_raw;

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

void checkDefaultParam()
{
    bool bNeedUpdateFlash = false;
    //if new version add parameters, we should set them to default
    u16 curVer = eeprom_buffer.params.firmware_ver;

    //v1.0.5              Released: 2015-6-15
    if(curVer < 5 || curVer == 0xFFFF)
    {
        bNeedUpdateFlash = true;

        eeprom_buffer.params.Atti_mp_posX = 180;
        eeprom_buffer.params.Atti_mp_posY = 133;
        eeprom_buffer.params.Atti_mp_scale_real = 1;
        eeprom_buffer.params.Atti_mp_scale_frac = 0;
        eeprom_buffer.params.Atti_3D_posX = 180;
        eeprom_buffer.params.Atti_3D_posY = 133;
        eeprom_buffer.params.Atti_3D_scale_real = 1;
        eeprom_buffer.params.Atti_3D_scale_frac = 0;
        eeprom_buffer.params.Atti_3D_map_radius = 40;
        eeprom_buffer.params.osd_offsetY = 0;
        eeprom_buffer.params.osd_offsetX = 0;
    }

    if (eeprom_buffer.params.osd_offsetX > 20) {
        eeprom_buffer.params.osd_offsetX = 20;
        bNeedUpdateFlash = true;
    }
    if (eeprom_buffer.params.osd_offsetX <-20) {
        eeprom_buffer.params.osd_offsetX = -20;
        bNeedUpdateFlash = true;
    }
    if (eeprom_buffer.params.osd_offsetY > 20) {
        eeprom_buffer.params.osd_offsetY = 20;
        bNeedUpdateFlash = true;
    }
    if (eeprom_buffer.params.osd_offsetY <-20) {
        eeprom_buffer.params.osd_offsetY = -20;
        bNeedUpdateFlash = true;
    }

    if (eeprom_buffer.params.firmware_ver < 6) {
        eeprom_buffer.params.firmware_ver = 6;
        bNeedUpdateFlash = true;
    }

    if (eeprom_buffer.params.firmware_ver < 7) {
        eeprom_buffer.params.firmware_ver = 7;
        eeprom_buffer.params.Speed_scale_posY = 133;
        eeprom_buffer.params.Alt_Scale_posY = 133;
        eeprom_buffer.params.BattConsumed_en = 1;
        eeprom_buffer.params.BattConsumed_panel = 1;
        eeprom_buffer.params.BattConsumed_posX = 350;
        eeprom_buffer.params.BattConsumed_posY = 34;
        eeprom_buffer.params.BattConsumed_fontsize = 0;
        eeprom_buffer.params.BattConsumed_align = 2;
        eeprom_buffer.params.TotalTripDist_en = 1;
        eeprom_buffer.params.TotalTripDist_panel = 1;
        eeprom_buffer.params.TotalTripDist_posX = 350;
        eeprom_buffer.params.TotalTripDist_posY = 210;
        eeprom_buffer.params.TotalTripDist_fontsize = 0;
        eeprom_buffer.params.TotalTripDist_align = 2;
        bNeedUpdateFlash = true;
    }

    bool ret = false;
    if(bNeedUpdateFlash)
    {
        ret = StoreParams();
        if(!ret)
        {
            //TODO - handle flash write error here
        }
    }
}
