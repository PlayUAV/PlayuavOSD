#ifndef __USART2_H
#define __USART2_H

#include "board.h"

#define MAVLINK_BUFFER_SIZE    128

void    mavlink_usart_init(uint32_t baudRate);
void    mavlink_buf_swap(void);
void    mavlink_usart_send_byte(u8 ch);

#endif //__USART2_H
