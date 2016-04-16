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

#include "usart2.h"
#include "osdconfig.h"

// Allocate buffers.
// Must be allocated in one block, so it is in a struct.
struct _buffers {
  uint8_t mavbuf_recv[MAVLINK_BUFFER_SIZE];
  uint8_t mavbuf_proc[MAVLINK_BUFFER_SIZE];
} mav_buffers;

// Remove the struct definition (makes it easier to write for).
#define mavbuf_recv (mav_buffers.mavbuf_recv)
#define mavbuf_proc  (mav_buffers.mavbuf_proc)

// Pointers to each of these buffers.
uint8_t *mavlink_buffer_recv;
uint8_t *mavlink_buffer_proc;
uint32_t mavlink_recv_pos = 0;

uint8_t protocol_start = 0xFE;

extern xSemaphoreHandle onMavlinkSemaphore;
extern xSemaphoreHandle onUAVTalkSemaphore;


// USART3 - MAVLINK (RX - IRQ, TX - IRQ)
void mavlink_usart_init(uint32_t baudRate) {
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  /* Configure and clear buffers */
  mavlink_buffer_recv = mavbuf_recv;
  mavlink_buffer_proc  = mavbuf_proc;
  memset(mavlink_buffer_recv, 0, MAVLINK_BUFFER_SIZE);
  memset(mavlink_buffer_proc, 0, MAVLINK_BUFFER_SIZE);

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  // USART3_TX    PB10
  // USART3_RX    PB11
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART1);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART1);

  //reset USART
  USART_DeInit(USART3);

  // RX interrupt, doesn't need high performance
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  USART_InitStructure.USART_BaudRate = baudRate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART3, &USART_InitStructure);
  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
  USART_Cmd(USART3, ENABLE);

  switch (eeprom_buffer.params.FC_Protocol) {
  case PROTOCOL_MAVLINK:
    //start with "0xFE"
    protocol_start = 0xFE;
    break;
  case PROTOCOL_UAVTALK:
    //start with "0x3C"
    protocol_start = 0x3C;
    break;
  default:
    break;
  }
}

// USART3 Rx IRQ Handler
void USART3_IRQHandler(void) {
  static portBASE_TYPE xHigherPriorityTaskWoken;
  xHigherPriorityTaskWoken = pdFALSE;

  uint8_t c;
  uint16_t SR = USART3->SR;

  if (SR & USART_FLAG_RXNE) {
    c = USART3->DR;

    //Denotes the start of mavlink message frame transmission
    if (c == protocol_start)
    {
      mavlink_buf_swap();
      mavlink_recv_pos = 0;
      memset(mavlink_buffer_recv, 0, MAVLINK_BUFFER_SIZE);
      switch (eeprom_buffer.params.FC_Protocol) {
      case PROTOCOL_MAVLINK:
        xSemaphoreGiveFromISR(onMavlinkSemaphore, &xHigherPriorityTaskWoken);
        break;
      case PROTOCOL_UAVTALK:
        xSemaphoreGiveFromISR(onUAVTalkSemaphore, &xHigherPriorityTaskWoken);
        break;
      default:
        break;
      }

    }
    mavlink_buffer_recv[mavlink_recv_pos++] = c;

    //avioding unknown protocol make the buffer overflow
    if (mavlink_recv_pos >= MAVLINK_BUFFER_SIZE)
      mavlink_recv_pos = 0;
  }

  portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void mavlink_buf_swap(void) {
  uint8_t *tmp;

  SWAP_BUFFS(tmp, mavlink_buffer_proc, mavlink_buffer_recv);
}

void mavlink_usart_send_byte(u8 ch) {
  while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET) {}
  USART_SendData(USART3, (u8) ch);
}
