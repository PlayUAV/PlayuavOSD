#ifndef __BOARD_H
#define __BOARD_H

//#define __USE_C99_MATH

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

#define NELEMENTS(x) (sizeof(x) / sizeof(*(x)))
#define TICKS2MS(t) ((t) * (portTICK_RATE_MS))
#define MS2TICKS(m) ((m) / (portTICK_RATE_MS))

#define R2D 	57.295779513082320876798154814105f						//180/PI
#define D2R 	0.017453292519943295769236907684886f					//PI/180
float Rad2Deg(float x);
float Deg2Rad(float x);

void Delay_us(u32 nus);

#define OSD_MASK_DMA 				DMA2_Stream3
#define OSD_MASK_DMA_IRQ			DMA2_Stream3_IRQn
#define OSD_MASK_SPI 				SPI1

#define OSD_LEVEL_DMA 				DMA1_Stream4
#define OSD_LEVEL_DMA_IRQ			DMA1_Stream4_IRQn
#define OSD_LEVEL_SPI 				SPI2

#define LINE_COUNTER_TIMER			TIM4
#define HSYNC_CAPTURE_TIMER		TIM2		
#define PIXEL_TIMER					TIM3
#define SPI_CLOSE_DELAY_TIMER    TIM12

#define USART1_RX_DMA				DMA2_Stream2
#define USART1_RX_DMA_IRQ			DMA2_Stream2_IRQn
#define USART1_TX_DMA				DMA2_Stream7
#define USART1_TX_DMA_IRQ			DMA2_Stream7_IRQn

#define USART2_RX_DMA				DMA1_Stream5
#define USART2_RX_DMA_IRQ			DMA1_Stream5_IRQn
#define USART2_TX_DMA				DMA1_Stream6
#define USART2_TX_DMA_IRQ			DMA1_Stream6_IRQn

#define USART3_RX_DMA				DMA1_Stream1
#define USART3_RX_DMA_IRQ			DMA1_Stream1_IRQn
#define USART3_TX_DMA				DMA1_Stream3
#define USART3_TX_DMA_IRQ			DMA1_Stream3_IRQn

#define VIDEO_SWITCH_TIMER			TIM5		//ch1->PA0
#define STACK_SIZE_MIN	512	// usStackDepth	- the stack size DEFINED IN WORDS.

enum thread_prio_e
{
	THREAD_PRIO_LOW = 1,
	THREAD_PRIO_NORMAL = 2,
	THREAD_PRIO_HIGH = 3,
	THREAD_PRIO_HIGHEST = 4, /* @note: this has to match (configMAX_PRIORITIES - 1) */
};

//all supported protocols
enum FC_Protocol
{
	PROTOCOL_MAVLINK = 0,
	PROTOCOL_UAVTALK = 1,
	PROTOCOL_MWII = 2,
	PROTOCOL_GPS = 3,
	//FC_DJI_NAZAV2 = 3,
};

//-------------------------
// Interrupt Priorities
//-------------------------
#define PIOS_IRQ_PRIO_LOW				12              // lower than RTOS
#define PIOS_IRQ_PRIO_MID				8               // higher than RTOS
#define PIOS_IRQ_PRIO_HIGH				5               // for SPI, ADC, I2C etc...
#define PIOS_IRQ_PRIO_HIGHEST			4               // for USART etc...

void board_init(void);
void module_init(void);

uint32_t GetSystimeMS(void);	//return the current system time

bool test_force_clear_all_params(void);

// Macro to swap buffers given a temporary pointer.
#define SWAP_BUFFS(tmp, a, b) { tmp = a; a = b; b = tmp; }

#endif /* __BOARD_H */

