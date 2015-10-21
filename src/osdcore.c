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
 * Tau Labs - Brain FPV Flight Controller(https://github.com/BrainFPV/TauLabs)
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
	.graphics_column_start = 105,   // First visible OSD column (after Hsync)
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



static const struct pios_video_type_cfg pios_video_type_telem = {
	.graphics_hight_real   = TELEM_LINES,   // telem rows
	.graphics_column_start = 0,   // uint8_t is too short for the value ( see below) !
	.graphics_line_start   = TELEM_START_LINE,    // First telem line
	.dma_buffer_length     = TELEM_BUFFER_WIDTH,  // DMA buffer length in bytes
	.period = 42 -1,        // -->PIXEL_TIMER->ARR assumes 84 MHz clock (TIM3 on APB1) so gives 500 nsec period
	.dc     = (42 / 2)-1,   // -->PIXEL_TIMER->CCR1 gives a duty cycle of 250 ns
};

/*
   The telem buffer stores data bits in  serial stream format
   as a stream of bits, each encoded byte consisting of { 1 start bit, 8 data bits and 1 stop bit}
   Each byte therefore needs 10 bits.
   The data is output to the mask level spi pin, while the level stays at white
   therefore video passthru gives a black level which represents a 'mark' data level
   and video masked out gives a white level, which repesents a 'space' data level
   
*/
static uint16_t const telem_graphics_hsync_capture_clks_start = 84 * 7; // 12 usec from Hsync first edge

// Allocate buffers.
// Must be allocated in one block, so it is in a struct.
struct _buffers {

    uint8_t buffer0_level[BUFFER_HEIGHT * BUFFER_WIDTH];
    uint8_t buffer0_mask[BUFFER_HEIGHT * BUFFER_WIDTH];
    uint8_t buffer1_level[BUFFER_HEIGHT * BUFFER_WIDTH];
    uint8_t buffer1_mask[BUFFER_HEIGHT * BUFFER_WIDTH];  

    uint8_t buffer_tele0[TELEM_LINES * TELEM_BUFFER_WIDTH];
    uint8_t buffer_tele1[TELEM_LINES * TELEM_BUFFER_WIDTH];
    uint8_t buffer_tele_level[TELEM_BUFFER_WIDTH];

} buffers;

// Remove the struct definition (makes it easier to write for).
#define buffer0_level (buffers.buffer0_level)
#define buffer0_mask  (buffers.buffer0_mask)
#define buffer1_level (buffers.buffer1_level)
#define buffer1_mask  (buffers.buffer1_mask)

#define buffer_tele0 (buffers.buffer_tele0)
#define buffer_tele1 (buffers.buffer_tele1)

// Pointers to each of these buffers.
uint8_t *draw_buffer_level;
uint8_t *draw_buffer_mask;

static uint8_t *disp_buffer_level;
static uint8_t *disp_buffer_mask;

uint8_t *write_buffer_tele;
static uint8_t *trans_buffer_tele;

static volatile uint8_t cur_trans_mode = trans_idle;
static volatile uint16_t active_line = 0;
static volatile uint8_t  end_of_lines = false;

const struct pios_video_type_boundary *pios_video_type_boundary_act = &pios_video_type_boundary_pal;

// Private variables
static int8_t x_offset = 0;
static int8_t y_offset = 0;
//static const struct pios_video_cfg *dev_cfg = NULL;
static uint16_t num_video_lines = 0;
static int8_t video_type_tmp = VIDEO_TYPE_PAL;
static int8_t video_type_act = VIDEO_TYPE_NONE;
static const struct pios_video_type_cfg *pios_video_type_cfg_act = &pios_video_type_cfg_pal;
static int32_t actual_osd_start_line = 0;
static int32_t actual_osd_end_line = 0;


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
    // PA6 == SOI1_MISO
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_6; // SPI1 MISO
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpio);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1 );

    // PA5  == SPI1_SCK
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_5; // SPI1 CLK slave
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpio);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1 );

    //OSD level pins
    // PC2 == SPI2_MISO
    gpio.GPIO_Pin = GPIO_Pin_2; // SPI2_MISO
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &gpio);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_SPI2 );

    // PB13 == SPI2_SCK
    gpio.GPIO_Pin = GPIO_Pin_13; // SPI2_SCK
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &gpio);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2 );

    // HSYNC capture timer: Start counting at HSYNC and start pixel timer after at correct x-position
    // PB3 == TIM2_CH2
    gpio.GPIO_Pin = GPIO_Pin_3;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &gpio);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_TIM2 );

    //HSYNC_CAPTURE_TIMER == TIM2
    // One shot
    // start trigger is TIM2_CH2 HSYNC (first edge? think 2nd edge from timing)
    // on update(overflo) sends TRGO to start PIXEL_TIMER (TIM3)
    TIM_TimeBaseStructInit(&tim);
    tim.TIM_Period = pios_video_type_cfg_act->dc
            * (pios_video_type_cfg_act->graphics_column_start + x_offset);
    tim.TIM_Prescaler = 0;
    tim.TIM_ClockDivision = 0;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(HSYNC_CAPTURE_TIMER, &tim);
    TIM_SelectOnePulseMode(HSYNC_CAPTURE_TIMER, TIM_OPMode_Single );
    TIM_SelectSlaveMode(HSYNC_CAPTURE_TIMER, TIM_SlaveMode_Trigger );
    TIM_SelectInputTrigger(HSYNC_CAPTURE_TIMER, TIM_TS_TI2FP2 );
    TIM_SelectMasterSlaveMode(HSYNC_CAPTURE_TIMER, TIM_MasterSlaveMode_Enable );
    TIM_SelectOutputTrigger(HSYNC_CAPTURE_TIMER, TIM_TRGOSource_Update );

    HSYNC_CAPTURE_TIMER->SMCR |= (1 << 7);

    //SPI_CLOSE_DELAY_TIMER
    // DMA provides an interrupt when last data is loaded to SPI DR regs
    // but then need to wait until the data is transmitted and busy is done to close SPI
    // Since we know how long it will take, use the timer
    // to cause an interrupt to close the SPI
    // rather than busy waiting in the DMA irq handler

    SPI_CLOSE_DELAY_TIMER->CR1 = (1 << 3) | (1 << 2);   // One pulse mode
    SPI_CLOSE_DELAY_TIMER->CR2 = 0;
    SPI_CLOSE_DELAY_TIMER->CNT = 0;
    SPI_CLOSE_DELAY_TIMER->PSC = 0;
    SPI_CLOSE_DELAY_TIMER->CCR1 = 0;
    SPI_CLOSE_DELAY_TIMER->CCR2 = 0;
    SPI_CLOSE_DELAY_TIMER->ARR = 0xFFFF;
    SPI_CLOSE_DELAY_TIMER->DIER = (1 << 0);

    nvic.NVIC_IRQChannel = TIM8_BRK_TIM12_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    // Pixel timer: Outputs clock for SPI
    gpio.GPIO_Pin = GPIO_Pin_4;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &gpio);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_TIM3 );

    TIM_OCStructInit(&timoc);
    timoc.TIM_OCMode = TIM_OCMode_PWM1;
    timoc.TIM_OutputState = TIM_OutputState_Enable;
    timoc.TIM_OutputNState = TIM_OutputNState_Disable;
    timoc.TIM_Pulse = 1;
    timoc.TIM_OCPolarity = TIM_OCPolarity_High;
    timoc.TIM_OCNPolarity = TIM_OCPolarity_High;
    timoc.TIM_OCIdleState = TIM_OCIdleState_Reset;
    timoc.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(PIXEL_TIMER, &timoc);
    TIM_OC1PreloadConfig(PIXEL_TIMER, TIM_OCPreload_Enable );
    TIM_SetCompare1(PIXEL_TIMER, pios_video_type_cfg_act->dc);
    TIM_SetAutoreload(PIXEL_TIMER, pios_video_type_cfg_act->period);
    TIM_ARRPreloadConfig(PIXEL_TIMER, ENABLE);
    TIM_CtrlPWMOutputs(PIXEL_TIMER, ENABLE);
    TIM_SelectInputTrigger(PIXEL_TIMER, TIM_TS_ITR1 );

    // Line counter: Counts number of HSYNCS (from hsync_capture) and triggers output of first visible line
    TIM_TimeBaseStructInit(&tim);
    tim.TIM_Period = 0xffff;
    tim.TIM_Prescaler = 0;
    tim.TIM_ClockDivision = 0;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(LINE_COUNTER_TIMER, &tim);

    nvic.NVIC_IRQChannel = TIM4_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH;
    nvic.NVIC_IRQChannelSubPriority = 1;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    // incs the count at start of drawn line
    TIM_SelectInputTrigger(LINE_COUNTER_TIMER, TIM_TS_ITR1 ); // ITR1 for TIM4 is TIM2 (HSYNC_CAPTURE_TIMER sends TRGO on update)
    // ITR1 in SMCR

    TIM_SelectSlaveMode(LINE_COUNTER_TIMER, TIM_SlaveMode_External1 );
    TIM_SelectOnePulseMode(LINE_COUNTER_TIMER, TIM_OPMode_Single );

    TIM_ITConfig(LINE_COUNTER_TIMER,
            TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3 | TIM_IT_CC4 | TIM_IT_COM
                    | TIM_IT_Trigger | TIM_IT_Break, DISABLE);

    // set  ccmr regs to outputs for match only
    LINE_COUNTER_TIMER->CCMR1 = 0;
    LINE_COUNTER_TIMER->CCMR2 = 0;

    // set up compare video row values for LINE_COUNTER compare irqs
    LINE_COUNTER_TIMER->CCR1 = TELEM_START_LINE;
    LINE_COUNTER_TIMER->CCR2 = TELEM_START_LINE + TELEM_LINES;
    LINE_COUNTER_TIMER->CCR3 = actual_osd_start_line;
    LINE_COUNTER_TIMER->CCR4 = actual_osd_end_line;

    TIM_Cmd(LINE_COUNTER_TIMER, DISABLE);

    // init OSD mask SPI
    SPI_StructInit(&spi);
    spi.SPI_Mode = SPI_Mode_Slave;
    spi.SPI_Direction = SPI_Direction_1Line_Tx;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_2Edge;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_Init(OSD_MASK_SPI, &spi);
    SPI_Init(OSD_LEVEL_SPI, &spi);

    // Configure DMA for SPI
    //  MASK  <-- SPI1_TX  on DMA2_Stream3.channel3
    //  LEVEL <-- SPI2_TX on  DMA1_Stream4.channel0
    DMA_StructInit(&dma);
    dma.DMA_Channel = DMA_Channel_3;
    dma.DMA_PeripheralBaseAddr = (uint32_t) &(SPI1 ->DR);
    dma.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    dma.DMA_BufferSize = BUFFER_WIDTH;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_VeryHigh;
    dma.DMA_FIFOMode = DMA_FIFOMode_Enable;
    dma.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    dma.DMA_MemoryBurst = DMA_MemoryBurst_INC4;
    dma.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(OSD_MASK_DMA, &dma);
    dma.DMA_Channel = DMA_Channel_0;
    dma.DMA_PeripheralBaseAddr = (uint32_t) &(SPI2 ->DR);
    DMA_Init(OSD_LEVEL_DMA, &dma);

    /* Trigger interrupt when transfer complete */
    DMA_ITConfig(OSD_MASK_DMA, DMA_IT_TC, ENABLE);
    DMA_ITConfig(OSD_LEVEL_DMA, DMA_IT_TC, ENABLE);

    /* Configure and clear buffers */
    draw_buffer_level = buffer0_level;
    draw_buffer_mask = buffer0_mask;
    disp_buffer_level = buffer1_level;
    disp_buffer_mask = buffer1_mask;

    write_buffer_tele = buffer_tele0;
    trans_buffer_tele = buffer_tele1;

    memset(disp_buffer_mask, 0, BUFFER_HEIGHT * BUFFER_WIDTH);
    memset(disp_buffer_level, 0, BUFFER_HEIGHT * BUFFER_WIDTH);
    memset(draw_buffer_mask, 0, BUFFER_HEIGHT * BUFFER_WIDTH);
    memset(draw_buffer_level, 0, BUFFER_HEIGHT * BUFFER_WIDTH);

    memset(write_buffer_tele, 0, TELEM_LINES * TELEM_BUFFER_WIDTH);
    memset(trans_buffer_tele, 0, TELEM_LINES * TELEM_BUFFER_WIDTH);
    memset(buffers.buffer_tele_level, 0xFF, TELEM_BUFFER_WIDTH);

    /* Configure DMA interrupt */
    nvic.NVIC_IRQChannel = OSD_MASK_DMA_IRQ;
    nvic.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    nvic.NVIC_IRQChannel = OSD_LEVEL_DMA_IRQ;
    NVIC_Init(&nvic);

    /* Enable SPI interrupts to DMA */
    SPI_I2S_DMACmd(OSD_MASK_SPI, SPI_I2S_DMAReq_Tx, ENABLE);
    SPI_I2S_DMACmd(OSD_LEVEL_SPI, SPI_I2S_DMAReq_Tx, ENABLE);

    // init and enable interrupts for vsync
    // PB1 VSYNC PIN
    // PB3 HSYNC
    gpio.GPIO_Pin = GPIO_PinSource1;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_Mode = GPIO_Mode_IN;
    gpio.GPIO_OType = GPIO_OType_OD;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &gpio);

    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource1 );

    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    nvic.NVIC_IRQChannel = EXTI1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    // Enable the capture timer
    TIM_Cmd(HSYNC_CAPTURE_TIMER, ENABLE);
}

//Vsync interrupt service routine
void EXTI1_IRQHandler()
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    static uint16_t Vsync_update = 0;

    if (EXTI_GetITStatus(EXTI_Line1 ) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line1 );

        // Stop the line counter
        TIM_Cmd(LINE_COUNTER_TIMER, DISABLE);

        // Update the number of video lines
        num_video_lines = LINE_COUNTER_TIMER->CNT;

        // check video type
        if (num_video_lines > VIDEO_TYPE_PAL_ROWS) {
            video_type_tmp = VIDEO_TYPE_PAL;
        }

        // if video type has changed set new active values
        if (video_type_act != video_type_tmp) {
            video_type_act = video_type_tmp;
            if (video_type_act == VIDEO_TYPE_NTSC) {
                pios_video_type_boundary_act = &pios_video_type_boundary_ntsc;
                pios_video_type_cfg_act = &pios_video_type_cfg_ntsc;
            } else {
                pios_video_type_boundary_act = &pios_video_type_boundary_pal;
                pios_video_type_cfg_act = &pios_video_type_cfg_pal;
            }
        }

        video_type_tmp = VIDEO_TYPE_NTSC;

        // Every VSYNC_REDRAW_CNT field: swap buffers and trigger redraw
        if (++Vsync_update >= VSYNC_REDRAW_CNT) {
            Vsync_update = 0;
            swap_buffers();
            xSemaphoreGiveFromISR(onScreenDisplaySemaphore,
                    &xHigherPriorityTaskWoken);
        }

        LINE_COUNTER_TIMER->SR = 0U;
        // new frame so restart line count
        LINE_COUNTER_TIMER->CNT = 0U;
        // enable CCR1 interrupt ( start of telem rows)
        LINE_COUNTER_TIMER->DIER = (1 << 1);
        TIM_Cmd(LINE_COUNTER_TIMER, ENABLE);
    }

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/**
 * Linecounter update interrupts  from CCR1/CCR2/CCR3/CCR4
 * Linecounter is clocked by  HSYNC + start of line timer ( HSYNC_CAPTURE_TIMER Update)
 */

// called from LINE_COUNTER CCR1 irq
static void start_telem_lines(void)
{
    PIXEL_TIMER->CCR1 = pios_video_type_telem.dc;
    PIXEL_TIMER->ARR = pios_video_type_telem.period;
    HSYNC_CAPTURE_TIMER->ARR = telem_graphics_hsync_capture_clks_start; // start of telem data output
    SPI_CLOSE_DELAY_TIMER->CNT = 0;
    SPI_CLOSE_DELAY_TIMER->ARR = (pios_video_type_telem.period + 1) * 8;
    SPI_CLOSE_DELAY_TIMER->SR = 0;

    cur_trans_mode = trans_tele;
    active_line = 0;
    end_of_lines = 0;
    // disable CCR1 interrupt and enable CCR2 interrupt
    LINE_COUNTER_TIMER->DIER = (1 << 2);
    prepare_line();
}

// called from LINE_COUNTER CCR2 irq
static void end_telem_lines(void)
{  
    // disable CCR2 interrupt and enable CCR3 interrupt
    LINE_COUNTER_TIMER->DIER = (1 << 3);
    // signal to DMA interrupt that we are done
    end_of_lines = true;
}

// called from LINE_COUNTER CCR3 irq
static void start_osd_lines(void)
{
    PIXEL_TIMER->CCR1 = pios_video_type_cfg_act->dc;
    PIXEL_TIMER->ARR = pios_video_type_cfg_act->period;
    HSYNC_CAPTURE_TIMER->ARR = pios_video_type_cfg_act->dc
            * (pios_video_type_cfg_act->graphics_column_start + x_offset);
    SPI_CLOSE_DELAY_TIMER->CNT = 0;
    SPI_CLOSE_DELAY_TIMER->ARR = (pios_video_type_cfg_act->period + 1) * 8;
    SPI_CLOSE_DELAY_TIMER->SR = 0;
    cur_trans_mode = trans_osd;
    active_line = 0;
    end_of_lines = 0;
    // disable CCR3 interrupt and enable CCR4 interrupt
    LINE_COUNTER_TIMER->DIER = (1 << 4);
    prepare_line();
}

// called from LINE_COUNTER CCR4 irq
static void end_osd_lines(void)
{  
    // disable CCR4 interrupt
    LINE_COUNTER_TIMER->DIER = 0;
    // signal to DMA interrupt that we are done
    end_of_lines = true;
}

void TIM4_IRQHandler(void)
{
    uint16_t const tim_sr = LINE_COUNTER_TIMER->SR & (0xF << 1);
    LINE_COUNTER_TIMER->SR = 0U;
    if (tim_sr & (1 << 1)) {
        start_telem_lines();
    } else {
        if (tim_sr & (1 << 2)) {
            end_telem_lines();
        } else {
            if (tim_sr & (1 << 3)) {
                start_osd_lines();
            } else {
                if (tim_sr & (1 << 4)) {
                    end_osd_lines();
                }
            }
        }
    }
}


void PIOS_VIDEO_DMA_Handler(void);
void DMA2_Stream3_IRQHandler(void) __attribute__((alias("PIOS_VIDEO_DMA_Handler")));
void DMA1_Stream4_IRQHandler(void) __attribute__((alias("PIOS_VIDEO_DMA_Handler")));

static void wait_spi_complete_then_disable(SPI_TypeDef * spi_ptr)
{
    for (;;) {
        uint16_t const sr = spi_ptr->SR;
        if ((sr & SPI_I2S_FLAG_TXE )&& (!( sr & SPI_I2S_FLAG_BSY )) ){
            spi_ptr->CR1 &= (uint16_t)~SPI_CR1_SPE;
            return;
        }
    }
}

 //  MASK  <-- SPI1_TX  on DMA2_Stream3.channel3
 //  LEVEL <-- SPI2_TX on  DMA1_Stream4.channel0

void TIM8_BRK_TIM12_IRQHandler(void)
{
    SPI_CLOSE_DELAY_TIMER->SR = 0;

    // if timing is right these should be done!
    wait_spi_complete_then_disable(OSD_MASK_SPI);
    wait_spi_complete_then_disable(OSD_LEVEL_SPI);

    if (end_of_lines) {
        DMA2 ->LIFCR |= DMA_FLAG_TCIF3;
        DMA1 ->HIFCR |= DMA_FLAG_TCIF4;
        PIXEL_TIMER->CR1 &= (uint16_t) ~TIM_CR1_CEN;
        PIXEL_TIMER->SMCR &= (uint16_t) ~TIM_SMCR_SMS;
        OSD_MASK_DMA->CR &= ~(uint32_t) DMA_SxCR_EN;
        OSD_LEVEL_DMA->CR &= ~(uint32_t) DMA_SxCR_EN;
        cur_trans_mode = trans_idle;
    } else {
        prepare_line();
    }
}
/**
 * DMA transfer complete interrupt handler
 * Note: This function is called for every line (~13k times / s), so we use direct register access for
 * efficiency
 */
void PIOS_VIDEO_DMA_Handler(void)
{	
   if( (DMA2->LISR & DMA_FLAG_TCIF3) && (DMA1->HISR & DMA_FLAG_TCIF4) ) {
      DMA2->LIFCR |= DMA_FLAG_TCIF3;
      DMA1->HIFCR |= DMA_FLAG_TCIF4;

      SPI_CLOSE_DELAY_TIMER->CNT = 0; 
      SPI_CLOSE_DELAY_TIMER->CR1 |= (1 << 0) ; // (CEN)
   }
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
    SWAP_BUFFS(tmp, trans_buffer_tele, write_buffer_tele);
}

/**
 * Prepare the system to watch for a Hsync pulse to trigger the pixel clock and clock out the next line
 * Note: This function is called for every line (~13k times / s), so we use direct register access for
 * efficiency
 */
 //  MASK  <-- SPI1_TX  on DMA2_Stream3.channel3
 //  LEVEL <-- SPI2_TX on  DMA1_Stream4.channel0
static inline void prepare_line(void)
{
    // Clear DMA interrupt flags
    DMA2 ->LIFCR |= DMA_FLAG_TCIF3 | DMA_FLAG_HTIF3 | DMA_FLAG_FEIF3
            | DMA_FLAG_TEIF3 | DMA_FLAG_DMEIF3;
    DMA1 ->HIFCR |= DMA_FLAG_TCIF4 | DMA_FLAG_HTIF4 | DMA_FLAG_FEIF4
            | DMA_FLAG_TEIF4 | DMA_FLAG_DMEIF4;
    // Prepare next line DMA:
    if (cur_trans_mode == trans_tele) {
        uint32_t const buf_offset = active_line * TELEM_BUFFER_WIDTH;

        OSD_MASK_DMA->M0AR = (uint32_t) &trans_buffer_tele[buf_offset];
        OSD_LEVEL_DMA->M0AR = (uint32_t) buffers.buffer_tele_level;

        OSD_MASK_DMA->NDTR = (uint16_t) TELEM_BUFFER_WIDTH;
        OSD_LEVEL_DMA->NDTR = (uint16_t) TELEM_BUFFER_WIDTH;
    } else { // osd
        uint32_t const buf_offset = active_line * BUFFER_WIDTH;

        OSD_MASK_DMA->M0AR = (uint32_t) &disp_buffer_mask[buf_offset];
        OSD_LEVEL_DMA->M0AR = (uint32_t) &disp_buffer_level[buf_offset];

        OSD_MASK_DMA->NDTR =
                (uint16_t) pios_video_type_cfg_act->dma_buffer_length;
        OSD_LEVEL_DMA->NDTR =
                (uint16_t) pios_video_type_cfg_act->dma_buffer_length;
    }
    // Advance line counter
    ++active_line;
    // Stop pixel timer
    PIXEL_TIMER->CR1 &= (uint16_t) ~TIM_CR1_CEN;
    // Set initial value
    PIXEL_TIMER->CNT = 0;
    // Reset the SMS bits
    PIXEL_TIMER->SMCR &= (uint16_t) ~TIM_SMCR_SMS;
    PIXEL_TIMER->SMCR |= TIM_SlaveMode_Trigger;
    // Enable SPI
    OSD_MASK_SPI->CR1 |= SPI_CR1_SPE;
    OSD_LEVEL_SPI->CR1 |= SPI_CR1_SPE;
    // Enable DMA
    OSD_MASK_DMA->CR |= (uint32_t) DMA_SxCR_EN;
    OSD_LEVEL_DMA->CR |= (uint32_t) DMA_SxCR_EN;
}

uint16_t osdVideoGetLines(void)
{
	return num_video_lines;
}

uint16_t osdVideoGetType(void)
{
	return video_type_act;
}

/**
*  Set the offset in x direction
*/
void osdVideoSetXOffset(int8_t x_offset_in)
{
	if (x_offset_in > 20)
		x_offset_in = 20;
	if (x_offset_in < -20)
		x_offset_in = -20;

	x_offset = x_offset_in;
}

/**
*  Set the offset in y direction
*/
void osdVideoSetYOffset(int8_t y_offset_in)
{
	if (y_offset_in > 20)
		y_offset_in = 20;
	if (y_offset_in < 0)    // we don't want to break the VBI
		y_offset_in = 0;
	y_offset = y_offset_in;

	actual_osd_start_line = pios_video_type_cfg_act->graphics_line_start + y_offset;
	actual_osd_end_line = pios_video_type_cfg_act->graphics_line_start + y_offset + pios_video_type_cfg_act->graphics_hight_real;
}
