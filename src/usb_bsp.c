/**
 ******************************************************************************
 * @file    usb_bsp.c
 * @author  MCD Application Team
 * @version V1.1.0
 * @date    19-March-2012
 * @brief   This file is responsible to offer board support package and is
 *          configurable by user.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

#include "usb_bsp.h"
#include "usbd_conf.h"

#include "misc.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

/**
 * @brief  USB_OTG_BSP_EnableInterrupt
 *         Enabele USB Global interrupt
 * @param  None
 * @retval None
 */
void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev) {
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  /* Configure SOF VBUS ID DM DP Pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8  |
                                GPIO_Pin_9  |
                                GPIO_Pin_11 |
                                GPIO_Pin_12;

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_OTG1_FS);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_OTG1_FS);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG1_FS);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG1_FS);

  /* this for ID line debug */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_OTG1_FS);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);
}

/**
 * @brief  USB_OTG_BSP_EnableInterrupt
 *         Enabele USB Global interrupt
 * @param  None
 * @retval None
 */
void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev) {
  NVIC_InitTypeDef NVIC_InitStructure;

  //NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief  USB_OTG_BSP_uDelay
 *         This function provides delay time in micro sec
 * @param  usec : Value of delay required in micro sec
 * @retval None
 */
void USB_OTG_BSP_uDelay(const uint32_t usec) {
  uint32_t count = 0;
  const uint32_t utime = (120 * usec / 7);
  do
  {
    if (++count > utime)
    {
      return;
    }
  }
  while (1);
}


/**
 * @brief  USB_OTG_BSP_mDelay
 *          This function provides delay time in milli sec
 * @param  msec : Value of delay required in milli sec
 * @retval None
 */
void USB_OTG_BSP_mDelay(const uint32_t msec) {
  USB_OTG_BSP_uDelay(msec * 1000);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
