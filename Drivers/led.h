#ifndef __LED_H
#define __LED_H

#include "board.h"

typedef enum 
{
  LED_GREEN = 0,
  LED_BLUE = 1,
} Led_TypeDef;

#define LEDn                             2

  
#define LED_GREEN_PIN                       GPIO_Pin_4
#define LED_GREEN_GPIO_PORT                 GPIOC
#define LED_GREEN_GPIO_CLK                  RCC_AHB1Periph_GPIOC
  
#define LED_BLUE_PIN                        GPIO_Pin_5
#define LED_BLUE_GPIO_PORT                  GPIOC
#define LED_BLUE_GPIO_CLK                   RCC_AHB1Periph_GPIOC
  

void STM_EVAL_LEDInit(Led_TypeDef Led);
void STM_EVAL_LEDOn(Led_TypeDef Led);
void STM_EVAL_LEDOff(Led_TypeDef Led);
void STM_EVAL_LEDToggle(Led_TypeDef Led);

#endif /* __LED_H */

