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
 
#include "osdconfig.h"
#include "board.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_cdc_vcp.h"

__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;

// protocol bytes
#define PROTO_INSYNC		0x12    // 'in sync' byte sent before status
#define PROTO_EOC			0x20    // end of command

// Reply bytes
#define PROTO_OK			0x10    // INSYNC/OK      - 'ok' response
#define PROTO_FAILED		0x11    // INSYNC/FAILED  - 'fail' response
#define PROTO_INVALID		0x13	// INSYNC/INVALID - 'invalid' response for bad commands

// Command bytes
#define PROTO_GET_SYNC		0x21    // NOP for re-establishing sync
#define PROTO_GET_DEVICE	0x22    // get device ID bytes
#define PROTO_CHIP_ERASE	0x23    // erase program area and reset program address
#define PROTO_START_TRANS	0x24	//
#define PROTO_SET_PARAMS	0x25
#define PROTO_GET_PARAMS	0x26
#define PROTO_DEVICE_BL_REV	0x27	// firmware revision
#define PROTO_END_TRANS		0x28
#define PROTO_SAVE_TO_FLASH	0x29
#define PROTO_BL_UPLOAD		0x55

static const uint8_t	bl_proto_rev = 1;	// value returned by PROTO_DEVICE_BL_REV
EEPROM_BUF_TYPE eeprom_buffer;

static const uint32_t usb_vcp_timeout = 1;

static void sync_response(void)
{
	uint8_t data[] = {
		PROTO_INSYNC,	// "in sync"
		PROTO_OK	// "OK"
	};

	VCP_send_str(data);
}

static void invalid_response(void)
{
	uint8_t data[] = {
		PROTO_INSYNC,	// "in sync"
		PROTO_INVALID	// "invalid command"
	};

	VCP_send_str(data);
}

static void failure_response(void)
{
	uint8_t data[] = {
		PROTO_INSYNC,	// "in sync"
		PROTO_FAILED	// "command failed"
	};

	VCP_send_str(data);
}

static int cin_wait(unsigned timeout)
{
    int ret = -1;
    u8 c;
    vTaskDelay( timeout / portTICK_RATE_MS );
    if (VCP_get_char(&c))
    {
        ret = (int)c;
    }
    return ret;
}
	
void vTaskVCP(void *pvParameters)
{
	static u32 address = 0;
	static uint8_t usb_inited = 0;
	uint8_t c;
	int arg;
	uint8_t tmpbuf[EERROM_SIZE];
	
	for(;;)
	{
	    //Give the change to the OS scheduling. It is really a fool idea. Change me!
	    //TODO - should use semaphore
	    vTaskDelay( 1 / portTICK_RATE_MS );
//		xSemaphoreTake(onVCPSemaphore, portMAX_DELAY);

		if(usb_inited == 0)
		{
		    GPIO_InitTypeDef  gpio;
            //This is a trick to perform a USB re-enumerate
            gpio.GPIO_Pin = GPIO_Pin_12;
            gpio.GPIO_Speed = GPIO_Speed_100MHz;
            gpio.GPIO_Mode = GPIO_Mode_OUT;
            gpio.GPIO_OType = GPIO_OType_PP;
            gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_Init(GPIOB, &gpio);
            GPIO_SetBits(GPIOB, GPIO_Pin_12 );
            vTaskDelay(500 / portTICK_RATE_MS);
            GPIO_ResetBits(GPIOB, GPIO_Pin_12 );

            // Initialize USB VCP. Do it ASAP
            USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb,
                    &USR_cb);
            usb_inited = 1;
		}

		if (VCP_get_char(&c))
		{
			switch(c) {
				case PROTO_GET_SYNC:
					if (cin_wait(usb_vcp_timeout) != PROTO_EOC)
						goto cmd_bad;
					break;
				case PROTO_GET_DEVICE:
					arg = cin_wait(usb_vcp_timeout);
					if (arg < 0)
						goto cmd_bad;
					if (cin_wait(usb_vcp_timeout) != PROTO_EOC)
						goto cmd_bad;
					
					switch (arg) {
						case PROTO_DEVICE_BL_REV:
						    VCP_send_str((uint8_t*)&bl_proto_rev);
							break;
						default:
							goto cmd_bad;
					}
			
					break;
				case PROTO_START_TRANS:
					if (cin_wait(usb_vcp_timeout) != PROTO_EOC)
						goto cmd_bad;
					address = 0;
					memset(tmpbuf,0,EERROM_SIZE);
					break;
				case PROTO_SET_PARAMS:
					arg = cin_wait(usb_vcp_timeout);
					if (arg < 0)
						goto cmd_bad;

					// sanity-check arguments
					if (arg % 4)
						goto cmd_bad;
					if ((address + arg) > EERROM_SIZE)
						goto cmd_bad;
					for (int i = 0; i < arg; i++) 
					{
						c = cin_wait(usb_vcp_timeout);
						if (c < 0)
							goto cmd_bad;
						
						tmpbuf[address++] = c;
					}
					
					break;
				case PROTO_END_TRANS:
					if (cin_wait(usb_vcp_timeout) != PROTO_EOC)
						goto cmd_bad;
					
					//ensure we receive right size
					if(address == EERROM_SIZE)
					{
						memcpy(eeprom_buffer.c,tmpbuf,EERROM_SIZE); 
//						bool ret = StoreParams();
//						if(!ret)
//						{
//							//TODO - handle flash write error here
//						}
					}
					break;
				case PROTO_SAVE_TO_FLASH:
					if (cin_wait(usb_vcp_timeout) != PROTO_EOC)
						goto cmd_bad;
					
					//ensure we receive right size
					if(address == EERROM_SIZE)
					{
						bool ret = StoreParams();
						if(!ret)
						{
							//TODO - handle flash write error here
						}
					}
					break;	
				case PROTO_GET_PARAMS:
					if (cin_wait(usb_vcp_timeout) != PROTO_EOC)
						goto cmd_bad;
					for (int i = 0; i < EERROM_SIZE; i++)
					{
					    VCP_put_char(eeprom_buffer.c[i]);
						//TM_USB_VCP_Putc(testbuf[i]);
						//vTaskDelay( 1 / portTICK_RATE_MS );
					}
					
					break;
				case PROTO_BL_UPLOAD:
					if (cin_wait(usb_vcp_timeout) != PROTO_EOC)
						goto cmd_bad;

					sync_response();
					vTaskDelay( 200 / portTICK_RATE_MS );
					
					GPIO_SetBits(GPIOB,GPIO_Pin_12);
					vTaskDelay( 500 / portTICK_RATE_MS );
					GPIO_ResetBits(GPIOB,GPIO_Pin_12);
					
					__DSB();  /* Ensure all outstanding memory accesses included
                                buffered write are completed before reset */
					SCB->AIRCR  = ((0x5FA << SCB_AIRCR_VECTKEY_Pos)      |
								 (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) |
								 SCB_AIRCR_SYSRESETREQ_Msk);                   /* Keep priority group unchanged */
					__DSB();                                                     /* Ensure completion of memory access */
					
					while(1);  
					break;
				default:
					continue;
							
			}
			
			// send the sync response for this command
			sync_response();
			continue;
		}
		
		vTaskDelay( 10 / portTICK_RATE_MS );
		continue;
cmd_bad:
		// send an 'invalid' response but don't kill the timeout - could be garbage
		invalid_response();
		continue;

cmd_fail:
		// send a 'command failed' response but don't kill the timeout - could be garbage
		failure_response();
		continue;		
	}
}

void LoadParams(void)
{
	uint32_t Address = 0;
	Address = EEPROM_START_ADDR;
	//read from flash
	for(int i=0; i<(EERROM_SIZE/4); i++)
	{
		eeprom_buffer.w[i] = *(__IO uint32_t*)Address;
		Address = Address + 4;
	}
	
	Address = 0;
}

bool StoreParams(void)
{
	uint32_t Address = 0;
	/* Unlock the Flash to enable the flash control register access *************/ 
	FLASH_Unlock();
	
	/* Clear pending flags (if any) */  
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
					FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
	
	//erase the whole sector
	/* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */ 
	if (FLASH_EraseSector(FLASH_Sector_11, VoltageRange_3) != FLASH_COMPLETE)
	{ 
		goto fop_error;
	}
	

	
	//Program the user Flash area word by word
	Address = EEPROM_START_ADDR;
	for(int i=0; i<(EERROM_SIZE/4); i++)
	{
		if (FLASH_ProgramWord(Address, eeprom_buffer.w[i]) == FLASH_COMPLETE)
		{
			Address = Address + 4;
		}
		else
		{ 
			goto fop_error;
		}
	}
	FLASH_Lock();
	return true;
	
fop_error:
		FLASH_Lock(); 
		return false;	
}

bool clear_all_params(void)
{
	/* Unlock the Flash to enable the flash control register access *************/ 
	FLASH_Unlock();
	
	/* Clear pending flags (if any) */  
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
					FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
	
	//erase the whole sector
	/* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */ 
	if (FLASH_EraseSector(FLASH_Sector_11, VoltageRange_3) != FLASH_COMPLETE)
	{ 
		goto fop_error;
	}

	FLASH_Lock();
	return true;
	
fop_error:
		FLASH_Lock(); 
		return false;	
}
bool flash_write_word(uint32_t add, uint32_t value)
{
	/* Unlock the Flash to enable the flash control register access *************/ 
	FLASH_Unlock();
	
	/* Clear pending flags (if any) */  
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
					FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
	
	if (FLASH_EraseSector(FLASH_Sector_11, VoltageRange_4) != FLASH_COMPLETE)
	{ 
		goto fop_error;
	}
	
	//Program the user Flash area word by word
	if (FLASH_ProgramWord(add, value) != FLASH_COMPLETE)
	{ 
		goto fop_error;
	}
	FLASH_Lock();
	return true;
	
fop_error:
		FLASH_Lock(); 
		return false;	
}

uint32_t get_map_bandrate(uint16_t mapbandrate)
{
    switch(mapbandrate)
    {
        case 1:
            return 4800;
        case 2:
            return 9600;
        case 3:
            return 19200;
        case 4:
            return 38400;
        case 5:
            return 43000;
        case 6:
            return 56000;
        case 7:
            return 57600;
        case 8:
            return 115200;
        default:
            return 57600;
    }

    return 57600;
}


