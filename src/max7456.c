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

#include "max7456.h" 
#include "spi.h"
#include "osdconfig.h"

u8 	max7456_startCol = 0;
u8 	max7456_startRow = 0;
u8 	max7456_col = 0;
u8 	max7456_row = 0;
u8 	max7456_videoMode = 0;


void SPI_MAX7456_init(void)
{	
	u8 osdbl_r = 0;
	u8 osdbl_w = 0;
	u32 x = 0;
	
 	MAX7456_CS_HIGH();

	SPI1_Init();
	
	SPI_MAX7456_setMode((int)eeprom_buffer.params.video_mode);
	
	MAX7456_CS_LOW();

	//read black level register
	SPI1_TransferByte(MAX7456_OSDBL_reg_read);//black level read register
	osdbl_r = SPI1_TransferByte(0xff);
	MAX7456_CS_HIGH();
	Delay_us(100);
	
	MAX7456_CS_LOW();
	Delay_us(100);
	SPI1_TransferByte(MAX7456_VM0_reg);
	SPI1_TransferByte(MAX7456_RESET | max7456_videoMode | 0x00010000);
	MAX7456_CS_HIGH();
	
	//set black level
	osdbl_w = (osdbl_r & 0xef); //Set bit 4 to zero 11101111
	SPI1_TransferByte(MAX7456_OSDBL_reg); //black level write register
	SPI1_TransferByte(osdbl_w);
	
	// set all rows to same charactor white level, 90%
	for (x = 0; x < MAX7456_screen_rows; x++)
	{
		SPI1_TransferByte(x + 0x10);
		SPI1_TransferByte(MAX7456_WHITE_level_120);
	}
	// define sync (auto,int,ext) and
	// making sure the Max7456 is enabled
	SPI_MAX7456_control(1);	
	SPI_MAX7456_clear();
	
	
	
}  

void SPI_MAX7456_control(u8 ctrl)
{
	MAX7456_CS_LOW();
	SPI1_TransferByte(MAX7456_VM0_reg);
	switch(ctrl)
	{
		case 0:
			SPI1_TransferByte(MAX7456_DISABLE_display | max7456_videoMode);
			break;
		case 1:
			SPI1_TransferByte((MAX7456_ENABLE_display_vert | max7456_videoMode) | MAX7456_SYNC_autosync);
			break;
	}
	MAX7456_CS_HIGH();
}

void SPI_MAX7456_detectMode(void)
{
//	MAX7456_CS_LOW();
//    //read STAT and auto detect Mode PAL/NTSC
//	SPI2_TransferByte(MAX7456_STAT_reg_read);//status register
//	u8 osdstat_r = SPI2_TransferByte(0xff);

//	if ((B00000001 & osdstat_r) == 1)
//	{
//		SPI_MAX7456_setMode(1);  
//    }
//    else if((B00000010 & osdstat_r) == 1)
//	{
//		SPI_MAX7456_setMode(0);
//	}
	SPI_MAX7456_setMode(0);
}

void SPI_MAX7456_setMode(int mode)
{
	switch(mode)
	{
		case 0:
			max7456_videoMode = MAX7456_MODE_MASK_NTCS;
			break;
		case 1:
			max7456_videoMode = MAX7456_MODE_MASK_PAL;
			break;
	}
}

void SPI_MAX7456_clear(void)
{
	// clear the screen
	MAX7456_CS_LOW();
	SPI1_TransferByte(MAX7456_DMM_reg);
	SPI1_TransferByte(MAX7456_CLEAR_display);
	MAX7456_CS_HIGH();
}

void SPI_MAX7456_setPanel(u8 start_col, u8 start_row)
{
	max7456_startCol = start_col;
	max7456_startRow = start_row;
	max7456_col = start_col;
	max7456_row = start_row;
}

void SPI_MAX7456_openPanel(void)
{
	u32 linepos;
	u8 settings, char_address_hi, char_address_lo;
	
//	io_redirect = IO_REDIRECT_MAX7456;
	
	//find [start address] position
	linepos = max7456_row*30+max7456_col;

	// divide 16 bits into hi & lo byte
	char_address_hi = linepos >> 8;
	char_address_lo = linepos;

	//Auto increment turn writing fast (less SPI commands).
	//No need to set next char address. Just send them
	settings = MAX7456_INCREMENT_auto; //To Enable DMM Auto Increment
	MAX7456_CS_LOW();
	SPI1_TransferByte(MAX7456_DMM_reg); //dmm
	SPI1_TransferByte(settings);

	SPI1_TransferByte(MAX7456_DMAH_reg); // set start address high
	SPI1_TransferByte(char_address_hi);

	SPI1_TransferByte(MAX7456_DMAL_reg); // set start address low
	SPI1_TransferByte(char_address_lo);
}

void SPI_MAX7456_writeSingle(uint8_t x, uint8_t y, u8 c)
{
	u32 linepos;
	uint8_t char_address_hi, char_address_lo;

	//find [start address] position
	linepos = y*30+x;

	// divide 16 bits into hi & lo uint8_t
	char_address_hi = linepos >> 8;
	char_address_lo = linepos;

	MAX7456_CS_LOW();

	SPI1_TransferByte(MAX7456_DMAH_reg); // set start address high
	SPI1_TransferByte(char_address_hi);

	SPI1_TransferByte(MAX7456_DMAL_reg); // set start address low
	SPI1_TransferByte(char_address_lo);

	SPI1_TransferByte(MAX7456_DMDI_reg);
	SPI1_TransferByte(c);

	MAX7456_CS_HIGH();
}

void SPI_MAX7456_closePanel(void)
{
	SPI1_TransferByte(MAX7456_DMDI_reg);
	//This is needed "trick" to finish auto increment
	SPI1_TransferByte(MAX7456_END_string); 
	MAX7456_CS_HIGH();
	//only after finish the auto increment the new row will really act as desired
	max7456_row++; 
//	io_redirect = IO_REDIRECT_USART;
}

void SPI_MAX7456_write(u8 c)
{
	if(c == '|'){
		SPI_MAX7456_closePanel(); //It does all needed to finish auto increment and change current row
		SPI_MAX7456_openPanel(); //It does all needed to re-enable auto increment
	}
	else{
		SPI1_TransferByte(MAX7456_DMDI_reg);
		SPI1_TransferByte(c);
	}
}

void SPI_MAX7456_write_NVM(u32 font_count, uint8_t *character_bitmap)
{
	u8 x;
	u8 char_address_hi, char_address_lo;
	u8 screen_char;

	char_address_hi = font_count;
	char_address_lo = 0;
	//Serial.println("write_new_screen");   

	// disable display
	MAX7456_CS_LOW();
	SPI1_TransferByte(MAX7456_VM0_reg); 
	SPI1_TransferByte(MAX7456_DISABLE_display);

	SPI1_TransferByte(MAX7456_CMAH_reg); // set start address high
	SPI1_TransferByte(char_address_hi);

	for(x = 0; x < NVM_ram_size; x++) // write out 54 (out of 64) bytes of character to shadow ram
	{
		screen_char = character_bitmap[x];
		SPI1_TransferByte(MAX7456_CMAL_reg); // set start address low
		SPI1_TransferByte(x);
		SPI1_TransferByte(MAX7456_CMDI_reg);
		SPI1_TransferByte(screen_char);
	}

	// transfer a 54 bytes from shadow ram to NVM
	SPI1_TransferByte(MAX7456_CMM_reg);
	SPI1_TransferByte(WRITE_nvr);

	// wait until bit 5 in the status register returns to 0 (12ms)
	while ((SPI1_TransferByte(MAX7456_STAT_reg_read) & STATUS_reg_nvr_busy) != 0x00);

	SPI1_TransferByte(MAX7456_VM0_reg); // turn on screen next vertical
	SPI1_TransferByte(MAX7456_ENABLE_display_vert);
	MAX7456_CS_HIGH();  
}

void read_one_char_from_NVM(u32 font_count)
{
	uint8_t x, tmp;
	uint8_t character_bitmap[NVM_ram_size];
	uint8_t char_address_hi;

	char_address_hi = font_count;

	MAX7456_CS_LOW();
	// disable display
	SPI1_TransferByte(MAX7456_VM0_reg); 
	SPI1_TransferByte(MAX7456_DISABLE_display);
	
	SPI1_TransferByte(MAX7456_CMAH_reg); // set start address high
	SPI1_TransferByte(char_address_hi);
	
	SPI1_TransferByte(MAX7456_CMM_reg); // set start address low
	SPI1_TransferByte(0x50);
	MAX7456_CS_HIGH();
	
	// wait until bit 5 in the status register returns to 0 (12ms)
	//MAX7456_CS_LOW();
	while ((SPI1_TransferByte(MAX7456_STAT_reg_read) & STATUS_reg_nvr_busy) != 0x00);
	//MAX7456_CS_HIGH();
	
	for(x = 0; x < NVM_ram_size; x++) // write out 54 (out of 64) uint8_ts of character to shadow ram
	{
		MAX7456_CS_LOW();
		SPI1_TransferByte(MAX7456_CMAL_reg); // set start address low
		SPI1_TransferByte(x);
		Delay_us(20);
		SPI1_TransferByte(MAX7456_CMDO_reg);
		character_bitmap[x] = SPI1_TransferByte(0xff);
		MAX7456_CS_HIGH();
		Delay_us(20);
	}

	MAX7456_CS_LOW();
	SPI1_TransferByte(MAX7456_VM0_reg); // turn on screen next vertical
	SPI1_TransferByte(MAX7456_ENABLE_display_vert);
	MAX7456_CS_HIGH();

	//for testing
	for(x = 0; x < NVM_ram_size; x++)
	{
		tmp = character_bitmap[x];
	}
	
}









