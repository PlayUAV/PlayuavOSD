#ifndef OSD_CORE_H__
#define OSD_CORE_H__

#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"

// PAL/NTSC specific boundary values
struct pios_video_type_boundary {
	uint16_t graphics_right;
	uint16_t graphics_bottom;
};

// PAL/NTSC specific config values
struct pios_video_type_cfg {
	uint16_t graphics_hight_real;
	uint8_t  graphics_column_start;
	uint8_t  graphics_line_start;
	uint8_t  dma_buffer_length;
	uint8_t  period;
	uint8_t  dc;
};

struct pios_osd_bw_cfg_t {
	TIM_TimeBaseInitTypeDef tim_base_init;
	TIM_OCInitTypeDef tim_oc_init;
	GPIO_InitTypeDef gpio_init;
	uint32_t remap;
	const struct pios_tim_channel * bw_channels;
};

//extern OS_FlagID osdUpdateFlag;
extern u8 OSD_need_update;

//extern void OSD_configure_bw_levels(void);
//extern void PWM_Output(void);
extern void osdCoreInit(void);

//extern void osdVideoSetLevels(uint8_t, uint8_t, uint8_t, uint8_t);
extern void osdVideoSetXOffset(int8_t);
extern void osdVideoSetYOffset(int8_t);

extern uint16_t osdVideoGetLines(void);
extern uint16_t osdVideoGetType(void);
extern void set_bw_levels(uint8_t black, uint8_t white);


// video boundary values
extern const struct pios_video_type_boundary *pios_video_type_boundary_act;
#define GRAPHICS_LEFT        0
#define GRAPHICS_TOP         0
#define GRAPHICS_RIGHT       pios_video_type_boundary_act->graphics_right
#define GRAPHICS_BOTTOM      pios_video_type_boundary_act->graphics_bottom

#define GRAPHICS_X_MIDDLE	((GRAPHICS_RIGHT + 1) / 2)
#define GRAPHICS_Y_MIDDLE	((GRAPHICS_BOTTOM + 1) / 2)


// video type defs for autodetect
#define VIDEO_TYPE_NONE      0
#define VIDEO_TYPE_NTSC      1
#define VIDEO_TYPE_PAL       2
#define VIDEO_TYPE_PAL_ROWS  300
#define VIDEO_TYPE_NTSC_ROWS  200


// draw area buffer values, for memory allocation, access and calculations we suppose the larger values for PAL, this also works for NTSC
#define GRAPHICS_WIDTH_REAL  376                            // max columns
#define GRAPHICS_HEIGHT_REAL 266                            // max lines
#define BUFFER_WIDTH         (GRAPHICS_WIDTH_REAL / 8 + 1)  // Bytes plus one byte for SPI, needs to be multiple of 4 for alignment
#define BUFFER_HEIGHT        (GRAPHICS_HEIGHT_REAL)

enum Trans_mode {
    trans_idle = 0,
    trans_tele = 1,
    trans_osd = 2,
};

// make sure that (START_LINE + NUMLINES) < 16 so doesnt overlap OSD ( NTSC osd starts on line 16)
// play safe to get in the  window for rx at the moment
#define TELEM_START_LINE    9    // first row of telem 
#define TELEM_LINES         4   // num telem rows 
#define TELEM_BUFFER_WIDTH  12  // gives (12 *8) div 10 == 9 bytes of data per line

#endif
