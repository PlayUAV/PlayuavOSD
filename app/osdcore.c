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
/* Standard includes. */
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "osdcore.h"
#include "graphengine.h"

extern xSemaphoreHandle onScreenDisplaySemaphore;

//OS_FlagID osdUpdateFlag;
u8 OSD_need_update = 0;
// How many frames until we redraw
#define VSYNC_REDRAW_CNT 2

static const struct pios_video_type_boundary pios_video_type_boundary_ntsc = {
	.graphics_right  = 351,         // must be: graphics_width_real - 1
	.graphics_bottom = 239,         // must be: graphics_hight_real - 1
};

static const struct pios_video_type_boundary pios_video_type_boundary_pal = {
	.graphics_right  = 359,         // must be: graphics_width_real - 1
	.graphics_bottom = 265,         // must be: graphics_hight_real - 1
};

static const struct pios_video_type_cfg pios_video_type_cfg_ntsc = {
	.graphics_hight_real   = 240,   // Real visible lines
	.graphics_column_start = 130,   // First visible OSD column (after Hsync)
	.graphics_line_start   = 16,    // First visible OSD line
	.dma_buffer_length     = 45,    // DMA buffer length in bytes (graphics_right / 8 + 1)
	.period = 11,
	.dc     = (11 / 2),
};

static const struct pios_video_type_cfg pios_video_type_cfg_pal = {
	.graphics_hight_real   = 266,   // Real visible lines
	.graphics_column_start = 120,   // First visible OSD column (after Hsync)
	.graphics_line_start   = 25,    // First visible OSD line
	.dma_buffer_length     = 46,    // DMA buffer length in bytes ((graphics_right + 1) / 8 + 1)
	.period = 10,
	.dc     = (10 / 2),
};

// Allocate buffers.
// Must be allocated in one block, so it is in a struct.
struct _buffers {
	uint8_t buffer0_level[BUFFER_HEIGHT * BUFFER_WIDTH];
	uint8_t buffer0_mask[BUFFER_HEIGHT * BUFFER_WIDTH];
	uint8_t buffer1_level[BUFFER_HEIGHT * BUFFER_WIDTH];
	uint8_t buffer1_mask[BUFFER_HEIGHT * BUFFER_WIDTH];
} buffers;

// Remove the struct definition (makes it easier to write for).
#define buffer0_level (buffers.buffer0_level)
#define buffer0_mask  (buffers.buffer0_mask)
#define buffer1_level (buffers.buffer1_level)
#define buffer1_mask  (buffers.buffer1_mask)

// Pointers to each of these buffers.
uint8_t *draw_buffer_level;
uint8_t *draw_buffer_mask;
uint8_t *disp_buffer_level;
uint8_t *disp_buffer_mask;

volatile uint16_t active_line = 0;

const struct pios_video_type_boundary *pios_video_type_boundary_act = &pios_video_type_boundary_pal;

// Private variables
static int8_t x_offset = 0;
static int8_t x_offset_new = 0;
static int8_t y_offset = 0;
//static const struct pios_video_cfg *dev_cfg = NULL;
static uint16_t num_video_lines = 0;
static int8_t video_type_tmp = VIDEO_TYPE_PAL;
static int8_t video_type_act = VIDEO_TYPE_NONE;
static const struct pios_video_type_cfg *pios_video_type_cfg_act = &pios_video_type_cfg_pal;

uint8_t black_pal = 30;
uint8_t white_pal = 110;
uint8_t black_ntsc = 10;
uint8_t white_ntsc = 110;

// Private functions
static void swap_buffers(void);
static void prepare_line(void);

void osdCoreInit(void)
{
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef tim;
    NVIC_InitTypeDef nvic;
    DMA_InitTypeDef dma;
    SPI_InitTypeDef spi;
    TIM_OCInitTypeDef timoc;
	EXTI_InitTypeDef EXTI_InitStructure;

    // OSD mask Pins
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_6; // SPI1 MISO
    gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpio);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
	
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_5; // SPI1 CLK slave
    gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
	gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpio);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
	
	//OSD level pins
	gpio.GPIO_Pin = GPIO_Pin_2; // SPI2_MISO
    gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &gpio);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_SPI2);
	
    gpio.GPIO_Pin = GPIO_Pin_13; // SPI2_SCK
    gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
	gpio.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &gpio);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
	
	// HSYNC captrue timer: Start counting at HSYNC and start pixel timer after at correct x-position
	gpio.GPIO_Pin = GPIO_Pin_3;
	gpio.GPIO_Speed = GPIO_Speed_100MHz;
	gpio.GPIO_Mode  = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_PP;
	//gpio.GPIO_PuPd  = GPIO_PuPd_UP;
	gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &gpio);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_TIM2);
	
	TIM_TimeBaseStructInit(&tim);
    tim.TIM_Period = pios_video_type_cfg_act->dc * (pios_video_type_cfg_act->graphics_column_start + x_offset);
    tim.TIM_Prescaler = 0;
    tim.TIM_ClockDivision = 0;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(HSYNC_CAPTURE_TIMER, &tim);
	TIM_SelectOnePulseMode(HSYNC_CAPTURE_TIMER, TIM_OPMode_Single);
	TIM_SelectSlaveMode(HSYNC_CAPTURE_TIMER, TIM_SlaveMode_Trigger);
	TIM_SelectInputTrigger(HSYNC_CAPTURE_TIMER, TIM_TS_TI2FP2);
	TIM_SelectMasterSlaveMode(HSYNC_CAPTURE_TIMER, TIM_MasterSlaveMode_Enable);
	TIM_SelectOutputTrigger(HSYNC_CAPTURE_TIMER, TIM_TRGOSource_Update);
	
	// Pixel timer: Outputs clock for SPI
	gpio.GPIO_Pin   = GPIO_Pin_4;
	gpio.GPIO_Speed = GPIO_Speed_100MHz;
	gpio.GPIO_Mode  = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &gpio);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_TIM3);
	
//	TIM_TimeBaseStructInit(&tim);
//	tim.TIM_Period = 10;
//	tim.TIM_Prescaler = 1;
//	tim.TIM_ClockDivision = 0;
//	tim.TIM_CounterMode = TIM_CounterMode_Up;
//	TIM_TimeBaseInit(PIXEL_TIMER, &tim);
	
	TIM_OCStructInit( &timoc );
    timoc.TIM_OCMode       = TIM_OCMode_PWM1;
	timoc.TIM_OutputState  = TIM_OutputState_Enable;
	timoc.TIM_OutputNState = TIM_OutputNState_Disable;
	timoc.TIM_Pulse        = 1;
	timoc.TIM_OCPolarity   = TIM_OCPolarity_High;
	timoc.TIM_OCNPolarity  = TIM_OCPolarity_High;
	timoc.TIM_OCIdleState  = TIM_OCIdleState_Reset;
	timoc.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(PIXEL_TIMER, &timoc );
	TIM_OC1PreloadConfig(PIXEL_TIMER, TIM_OCPreload_Enable);
	TIM_SetCompare1(PIXEL_TIMER, pios_video_type_cfg_act->dc);
	TIM_SetAutoreload(PIXEL_TIMER, pios_video_type_cfg_act->period);
	TIM_ARRPreloadConfig(PIXEL_TIMER, ENABLE);
	TIM_CtrlPWMOutputs(PIXEL_TIMER, ENABLE);
	TIM_SelectInputTrigger(PIXEL_TIMER, TIM_TS_ITR1);
	
	// Line counter: Counts number of HSYNCS (from hsync_capture) and triggers output of first visible line
	TIM_TimeBaseStructInit(&tim);
	tim.TIM_Period = 0xffff;
	tim.TIM_Prescaler = 0;
	tim.TIM_ClockDivision = 0;
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(LINE_COUNTER_TIMER, &tim);
	
	/* Enable the TIM4 gloabal Interrupt */
	nvic.NVIC_IRQChannel = TIM4_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST;
	nvic.NVIC_IRQChannelSubPriority = 1;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
	TIM_SelectInputTrigger(LINE_COUNTER_TIMER, TIM_TS_ITR1);
	TIM_SelectSlaveMode(LINE_COUNTER_TIMER, TIM_SlaveMode_External1);
	TIM_SelectOnePulseMode(LINE_COUNTER_TIMER, TIM_OPMode_Single);
	TIM_ITConfig(LINE_COUNTER_TIMER, TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3 | TIM_IT_CC4 | TIM_IT_COM | TIM_IT_Trigger | TIM_IT_Break, DISABLE);
	TIM_Cmd(LINE_COUNTER_TIMER, DISABLE);
	
    // init OSD mask SPI
    SPI_StructInit(&spi);
    spi.SPI_Mode              = SPI_Mode_Slave;
	spi.SPI_Direction         = SPI_Direction_1Line_Tx;
	spi.SPI_DataSize          = SPI_DataSize_8b;
	spi.SPI_NSS               = SPI_NSS_Soft;
	spi.SPI_FirstBit          = SPI_FirstBit_MSB;
	spi.SPI_CRCPolynomial     = 7;
	spi.SPI_CPOL              = SPI_CPOL_Low;
	spi.SPI_CPHA              = SPI_CPHA_2Edge;
	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_Init(OSD_MASK_SPI, &spi);
	
	SPI_StructInit(&spi);
    spi.SPI_Mode              = SPI_Mode_Slave;
	spi.SPI_Direction         = SPI_Direction_1Line_Tx;
	spi.SPI_DataSize          = SPI_DataSize_8b;
	spi.SPI_NSS               = SPI_NSS_Soft;
	spi.SPI_FirstBit          = SPI_FirstBit_MSB;
	spi.SPI_CRCPolynomial     = 7;
	spi.SPI_CPOL              = SPI_CPOL_Low;
	spi.SPI_CPHA              = SPI_CPHA_2Edge;
	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_Init(OSD_LEVEL_SPI, &spi);

    // Configure DMA for SPI - MASK_DMA DMA1_Channel3, LEVEL_DMA DMA1_Channel5
    DMA_StructInit(&dma);
	dma.DMA_Channel 		   = DMA_Channel_3;
    dma.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
	dma.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
	dma.DMA_BufferSize         = BUFFER_WIDTH;
	dma.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
	dma.DMA_MemoryInc          = DMA_MemoryInc_Enable;
	dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	dma.DMA_MemoryDataSize     = DMA_MemoryDataSize_Word;
	dma.DMA_Mode               = DMA_Mode_Normal;
	dma.DMA_Priority           = DMA_Priority_VeryHigh;
	dma.DMA_FIFOMode           = DMA_FIFOMode_Enable;
	dma.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
	dma.DMA_MemoryBurst        = DMA_MemoryBurst_INC4;
	dma.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
    DMA_Init(OSD_MASK_DMA, &dma);
	
	dma.DMA_Channel 		   = DMA_Channel_0;
	dma.DMA_PeripheralBaseAddr = (uint32_t)&(SPI2->DR);
	dma.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
	dma.DMA_BufferSize         = BUFFER_WIDTH;
	dma.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
	dma.DMA_MemoryInc          = DMA_MemoryInc_Enable;
	dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	dma.DMA_MemoryDataSize     = DMA_MemoryDataSize_Word;
	dma.DMA_Mode               = DMA_Mode_Normal;
	dma.DMA_Priority           = DMA_Priority_VeryHigh;
	dma.DMA_FIFOMode           = DMA_FIFOMode_Enable;
	dma.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
	dma.DMA_MemoryBurst        = DMA_MemoryBurst_INC4;
	dma.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
	DMA_Init(OSD_LEVEL_DMA, &dma);
	
	/* Trigger interrupt when transfer complete */
	DMA_ITConfig(OSD_MASK_DMA, DMA_IT_TC, ENABLE);
	DMA_ITConfig(OSD_LEVEL_DMA, DMA_IT_TC, ENABLE);
	
	/* Configure and clear buffers */
	draw_buffer_level = buffer0_level;
	draw_buffer_mask  = buffer0_mask;
	disp_buffer_level = buffer1_level;
	disp_buffer_mask  = buffer1_mask;
	memset(disp_buffer_mask, 0, BUFFER_HEIGHT * BUFFER_WIDTH);
	memset(disp_buffer_level, 0, BUFFER_HEIGHT * BUFFER_WIDTH);
	memset(draw_buffer_mask, 0, BUFFER_HEIGHT * BUFFER_WIDTH);
	memset(draw_buffer_level, 0, BUFFER_HEIGHT * BUFFER_WIDTH);
	
    /* Configure DMA interrupt */
	nvic.NVIC_IRQChannel = OSD_MASK_DMA_IRQ;
  	nvic.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST;
  	nvic.NVIC_IRQChannelSubPriority = 0;
  	nvic.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&nvic); 
	nvic.NVIC_IRQChannel = OSD_LEVEL_DMA_IRQ;
  	nvic.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST;
  	nvic.NVIC_IRQChannelSubPriority = 0;
  	nvic.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&nvic); 
	
	/* Enable SPI interrupts to DMA */
	SPI_I2S_DMACmd(OSD_MASK_SPI, SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_I2S_DMACmd(OSD_LEVEL_SPI, SPI_I2S_DMAReq_Tx, ENABLE);
	
	// init and enable interrupts for vsync
	gpio.GPIO_Pin = GPIO_PinSource1;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
	gpio.GPIO_Mode  = GPIO_Mode_IN;
	gpio.GPIO_OType = GPIO_OType_OD;
	gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &gpio);
	
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource1);
	
	EXTI_InitStructure.EXTI_Line=EXTI_Line1;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);
	
	nvic.NVIC_IRQChannel = EXTI1_IRQn;
  	nvic.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST;
  	nvic.NVIC_IRQChannelSubPriority = 0;
  	nvic.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&nvic); 
	
	// Enable hsync interrupts
	TIM_ITConfig(LINE_COUNTER_TIMER, TIM_IT_Update, ENABLE);
	
	// Enable the capture timer
	TIM_Cmd(HSYNC_CAPTURE_TIMER, ENABLE);
}

//void osdCoreDeinit(void)
//{
//	TIM_Cmd(LINE_COUNTER_TIMER, DISABLE);
//	TIM_ITConfig(LINE_COUNTER_TIMER, TIM_IT_Update, DISABLE);
//	TIM_Cmd(HSYNC_CAPTURE_TIMER, DISABLE);
//	
//	vPortEnterCritical();
//	osd_inited = 0;
//	vPortExitCritical();
//}

//Vsync interrupt service routine
void EXTI1_IRQHandler()
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	
	static uint16_t Vsync_update = 0;
	
//	vPortEnterCritical();
//	taskENTER_CRITICAL();
	
	if(EXTI_GetITStatus(EXTI_Line1) != RESET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line1);
		
		// Stop the line counter
		TIM_Cmd(LINE_COUNTER_TIMER, DISABLE);

		// Update the number of video lines
		num_video_lines = LINE_COUNTER_TIMER->CNT;

		// check video type
		if (num_video_lines > VIDEO_TYPE_PAL_ROWS) 
		{
			video_type_tmp = VIDEO_TYPE_PAL;
		}

		// if video type has changed set new active values
		if (video_type_act != video_type_tmp) 
		{
			video_type_act = video_type_tmp;
			if (video_type_act == VIDEO_TYPE_NTSC) 
			{
				pios_video_type_boundary_act = &pios_video_type_boundary_ntsc;
				pios_video_type_cfg_act = &pios_video_type_cfg_ntsc;
				//set_bw_levels(black_ntsc, white_ntsc);
			} 
			else 
			{
				pios_video_type_boundary_act = &pios_video_type_boundary_pal;
				pios_video_type_cfg_act = &pios_video_type_cfg_pal;
				//set_bw_levels(black_pal, white_pal);
			}
			PIXEL_TIMER->CCR1 = pios_video_type_cfg_act->dc;
			PIXEL_TIMER->ARR  = pios_video_type_cfg_act->period;
			HSYNC_CAPTURE_TIMER->ARR = pios_video_type_cfg_act->dc * (pios_video_type_cfg_act->graphics_column_start + x_offset);
		}
		if (x_offset != x_offset_new)
		{
			x_offset = x_offset_new;
			HSYNC_CAPTURE_TIMER->ARR = pios_video_type_cfg_act->dc * (pios_video_type_cfg_act->graphics_column_start + x_offset);
		}

		video_type_tmp = VIDEO_TYPE_NTSC;


		// Every VSYNC_REDRAW_CNT field: swap buffers and trigger redraw
		if (++Vsync_update >= VSYNC_REDRAW_CNT) {
			if((LINE_COUNTER_TIMER->DIER & TIM_IT_Update) == (uint16_t)RESET)
			{
				TIM_ITConfig(LINE_COUNTER_TIMER, TIM_IT_Update, ENABLE);
			}
			Vsync_update = 0;
			swap_buffers();
			xSemaphoreGiveFromISR(onScreenDisplaySemaphore, &xHigherPriorityTaskWoken);
		}
		
		// Get ready for the first line
		active_line = 0;

		// Set the number of lines to wait until we start clocking out pixels
		LINE_COUNTER_TIMER->CNT = 0xffff - (pios_video_type_cfg_act->graphics_line_start + y_offset);
		TIM_Cmd(LINE_COUNTER_TIMER, ENABLE);
		

	}
	
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
//	vPortExitCritical();
//	taskEXIT_CRITICAL();
}

/**
 * ISR Triggered by line_counter, starts clocking out pixels for first visible OSD line
 */
void TIM4_IRQHandler(void)
{
//	vPortEnterCritical();
//	taskENTER_CRITICAL();
	
	if(TIM_GetITStatus(LINE_COUNTER_TIMER, TIM_IT_Update) && (active_line == 0))
	{
		// Clear the interrupt flag
		LINE_COUNTER_TIMER->SR &= ~TIM_SR_UIF;

		// Prepare the first line
		prepare_line();

		// Hack: The timing for the first line is critical, so we output it again
		active_line = 0;

		// Get ready to count the remaining lines
		LINE_COUNTER_TIMER->CNT = pios_video_type_cfg_act->graphics_line_start + y_offset;
		TIM_Cmd(LINE_COUNTER_TIMER, ENABLE);
	}
//	vPortExitCritical();
//	taskEXIT_CRITICAL();
}

void PIOS_VIDEO_DMA_Handler(void);
void DMA2_Stream3_IRQHandler(void) __attribute__((alias("PIOS_VIDEO_DMA_Handler")));
void DMA1_Stream4_IRQHandler(void) __attribute__((alias("PIOS_VIDEO_DMA_Handler")));

/**
 * DMA transfer complete interrupt handler
 * Note: This function is called for every line (~13k times / s), so we use direct register access for
 * efficiency
 */
void PIOS_VIDEO_DMA_Handler(void)
{	
//	vPortEnterCritical();
//	taskENTER_CRITICAL();
	// Handle flags from DMA stream channel
	if ((DMA2->LISR & DMA_FLAG_TCIF3) && (DMA1->HISR & DMA_FLAG_TCIF4)) {

		// Flush the SPI
		while ((OSD_LEVEL_SPI->SR & SPI_I2S_FLAG_TXE) == 0) {
			;
		}
		while (OSD_LEVEL_SPI->SR & SPI_I2S_FLAG_BSY) {
			;
		}
		while ((OSD_MASK_SPI->SR & SPI_I2S_FLAG_TXE) == 0) {
			;
		}
		while (OSD_MASK_SPI->SR & SPI_I2S_FLAG_BSY) {
			;
		}

		// Disable the SPI, makes sure the pins are LOW
		OSD_MASK_SPI->CR1 &= (uint16_t)~SPI_CR1_SPE;
		OSD_LEVEL_SPI->CR1 &= (uint16_t)~SPI_CR1_SPE;

		if (active_line < pios_video_type_cfg_act->graphics_hight_real) { // lines existing
			prepare_line();
		} else { // last line completed
			// Clear the DMA interrupt flags
			DMA2->LIFCR  |= DMA_FLAG_TCIF3;
			DMA1->HIFCR |= DMA_FLAG_TCIF4;

			// Stop pixel timer
			PIXEL_TIMER->CR1  &= (uint16_t) ~TIM_CR1_CEN;

			// Disable the pixel timer slave mode configuration
			PIXEL_TIMER->SMCR &= (uint16_t) ~TIM_SMCR_SMS;
			// Stop DMA
			OSD_MASK_DMA->CR  &= ~(uint32_t)DMA_SxCR_EN;
			OSD_LEVEL_DMA->CR &= ~(uint32_t)DMA_SxCR_EN;
		}
	}
//	vPortExitCritical();
//	taskEXIT_CRITICAL();
}

/**
 * swap_buffers: Swaps the two buffers. Contents in the display
 * buffer is seen on the output and the display buffer becomes
 * the new draw buffer.
 */
static void swap_buffers(void)
{
	// While we could use XOR swap this is more reliable and
	// dependable and it's only called a few times per second.
	// Many compilers should optimize these to EXCH instructions.
	uint8_t *tmp;

	SWAP_BUFFS(tmp, disp_buffer_mask, draw_buffer_mask);
	SWAP_BUFFS(tmp, disp_buffer_level, draw_buffer_level);
}

/**
 * Prepare the system to watch for a Hsync pulse to trigger the pixel clock and clock out the next line
 * Note: This function is called for every line (~13k times / s), so we use direct register access for
 * efficiency
 */
static inline void prepare_line(void)
{
	uint32_t buf_offset = active_line * BUFFER_WIDTH;

	// Prepare next line DMA:
	// Clear DMA interrupt flags
	DMA2->LIFCR  |= DMA_FLAG_TCIF3 | DMA_FLAG_HTIF3 | DMA_FLAG_FEIF3 | DMA_FLAG_TEIF3 | DMA_FLAG_DMEIF3;
	DMA1->HIFCR |= DMA_FLAG_TCIF4 | DMA_FLAG_HTIF4 | DMA_FLAG_FEIF4 | DMA_FLAG_TEIF4 | DMA_FLAG_DMEIF4;
	
	// Load new line
	OSD_MASK_DMA->M0AR  = (uint32_t)&disp_buffer_mask[buf_offset];
	OSD_LEVEL_DMA->M0AR = (uint32_t)&disp_buffer_level[buf_offset];
	// Set length
	OSD_MASK_DMA->NDTR  = (uint16_t)pios_video_type_cfg_act->dma_buffer_length;
	OSD_LEVEL_DMA->NDTR = (uint16_t)pios_video_type_cfg_act->dma_buffer_length;

	// Stop pixel timer
	PIXEL_TIMER->CR1  &= (uint16_t) ~TIM_CR1_CEN;

	// Set initial value
	PIXEL_TIMER->CNT   = 0;

	// Reset the SMS bits
	PIXEL_TIMER->SMCR &= (uint16_t) ~TIM_SMCR_SMS;
	PIXEL_TIMER->SMCR |= TIM_SlaveMode_Trigger;

	// Enable SPI
	OSD_MASK_SPI->CR1  |= SPI_CR1_SPE;
	OSD_LEVEL_SPI->CR1 |= SPI_CR1_SPE;

	// Enable DMA
	OSD_MASK_DMA->CR  |= (uint32_t)DMA_SxCR_EN;
	OSD_LEVEL_DMA->CR |= (uint32_t)DMA_SxCR_EN;
	// Advance line counter
	active_line++;
}

uint16_t osdVideoGetLines(void)
{
	return num_video_lines;
}

uint16_t osdVideoGetType(void)
{
	return video_type_act;
}

///**
//*  Set the black and white levels
//*/
//void osdVideoSetLevels(uint8_t black_pal_in, uint8_t white_pal_in, uint8_t black_ntsc_in, uint8_t white_ntsc_in)
//{
//	if (video_type_act == VIDEO_TYPE_PAL)
//		set_bw_levels(black_pal_in, white_pal_in);
//	else
//		set_bw_levels(black_ntsc_in, white_ntsc_in);
//	black_pal = black_pal_in;
//	white_pal = white_pal_in;
//	black_ntsc = black_ntsc_in;
//	white_ntsc = white_ntsc_in;
//}

/**
*  Set the offset in x direction
*/
void osdVideoSetXOffset(int8_t x_offset_in)
{
	if (x_offset_in > 20)
		x_offset_in = 20;
	if (x_offset_in < -20)
		x_offset_in = -20;

	x_offset_new = x_offset_in;
	//dev_cfg->hsync_capture.timer->ARR = pios_video_type_cfg_act->dc * (pios_video_type_cfg_act->graphics_column_start + x_offset);
}

/**
*  Set the offset in y direction
*/
void osdVideoSetYOffset(int8_t y_offset_in)
{
	if (y_offset_in > 20)
		y_offset_in = 20;
	if (y_offset_in < -20)
		y_offset_in = -20;
	y_offset = y_offset_in;
}

//void set_bw_levels(uint8_t black, uint8_t white)
//{
//	TIM1->CCR1 = black;
//	TIM1->CCR3 = white;
//}

//void OSD_configure_bw_levels(void)
//{
//	GPIO_InitTypeDef GPIO_InitStructure;
//	TIM_OCInitTypeDef  TIM_OCInitStructure;
//	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

//	/* --------------------------- System Clocks Configuration -----------------*/
//	/* TIM1 clock enable */
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

//	/* GPIOA clock enable */
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

//	/* Connect TIM1 pins to AF */
//	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);
//	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);

//	/*-------------------------- GPIO Configuration ----------------------------*/
//	GPIO_StructInit(&GPIO_InitStructure); // Reset init structure
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_10;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);


//	/* Time base configuration */
//	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
//	TIM_TimeBaseStructure.TIM_Prescaler = (SystemCoreClock / 25500000) - 1; // Get clock to 25 MHz on STM32F2/F4
//	TIM_TimeBaseStructure.TIM_Period = 255;
//	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
//	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
//	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

//	/* Enable TIM1 Preload register on ARR */
//	TIM_ARRPreloadConfig(TIM1, ENABLE);

//	/* TIM PWM1 Mode configuration */
//	TIM_OCStructInit(&TIM_OCInitStructure);
//	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
//	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
//	TIM_OCInitStructure.TIM_Pulse = 90;
//	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

//	/* Output Compare PWM1 Mode configuration: Channel1 PA.08 */
//	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
//	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
//	TIM_OC3Init(TIM1, &TIM_OCInitStructure);
//	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

//	/* TIM1 Main Output Enable */
//	TIM_CtrlPWMOutputs(TIM1, ENABLE);

//	/* TIM1 enable counter */
//	TIM_Cmd(TIM1, ENABLE);
//	TIM1->CCR1 = 30;
//	TIM1->CCR3 = 110;
//}

//void PWM_Output(void)
//{
//	GPIO_InitTypeDef GPIO_InitStructure;
//	TIM_OCInitTypeDef  TIM_OCInitStructure;
//	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

//	/* --------------------------- System Clocks Configuration -----------------*/
//	/* TIM clock enable */
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 | RCC_APB2Periph_TIM8, ENABLE);

//	/* GPIO clock enable */
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC, ENABLE);

//	/*-------------------------- GPIO Configuration ----------------------------*/
//	GPIO_StructInit(&GPIO_InitStructure); // Reset init structure
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//	//GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOC, &GPIO_InitStructure);
//	
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);

//	GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_TIM8);		//v
//	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);    //h
//	
//	/* Time base configuration */
//	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
//	TIM_TimeBaseStructure.TIM_Prescaler = 0; 
//	TIM_TimeBaseStructure.TIM_Period = 10751;
//	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
//	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
//	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

//	/* Enable TIM1 Preload register on ARR */
//	TIM_ARRPreloadConfig(TIM1, ENABLE);

//	/* TIM PWM1 Mode configuration */
//	TIM_OCStructInit(&TIM_OCInitStructure);
//	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
//	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
//	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputState_Enable;
//	TIM_OCInitStructure.TIM_Pulse = 1;
//	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
//	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_Low;
//	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
//	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;

//	/* Output Compare PWM1 Mode configuration: Channel1 PA.08 */
//	TIM_OC3Init(TIM1, &TIM_OCInitStructure);
//	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

//	/* TIM1 Main Output Enable */
//	TIM_CtrlPWMOutputs(TIM1, ENABLE);

//	/* TIM1 enable counter */
//	TIM_Cmd(TIM1, ENABLE);
//	//TIM1->CCR3 = 672;
//	TIM1->CCR3 = 10079;
//	
//	/* Time base configuration */
//	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
//	TIM_TimeBaseStructure.TIM_Prescaler = 167; 
//	TIM_TimeBaseStructure.TIM_Period = 19999;
//	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
//	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
//	TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);

//	/* Enable TIM1 Preload register on ARR */
//	TIM_ARRPreloadConfig(TIM8, ENABLE);

//	/* TIM PWM1 Mode configuration */
//	TIM_OCStructInit(&TIM_OCInitStructure);
//	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
//	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
//	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputState_Enable;
//	TIM_OCInitStructure.TIM_Pulse = 1;
//	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
//	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_Low;
//	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
//	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;

//	/* Output Compare PWM1 Mode configuration: Channel1 PA.08 */
//	TIM_OC3Init(TIM8, &TIM_OCInitStructure);
//	TIM_OC3PreloadConfig(TIM8, TIM_OCPreload_Enable);

//	/* TIM1 Main Output Enable */
//	TIM_CtrlPWMOutputs(TIM8, ENABLE);

//	/* TIM1 enable counter */
//	TIM_Cmd(TIM8, ENABLE);
//	//TIM8->CCR3 = 199;
//	TIM8->CCR3 = 19800;
//}
