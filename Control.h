#ifndef	_CONTROL
#define _CONTROL
#include "stm32f4xx.h"
#include <stdint.h>
#include "MB_RTU_Master.h"
//discovery
//#define LED_PORT		GPIOD
//#define LED1_PIN		GPIO_ODR_ODR_12
//#define LED2_PIN		GPIO_ODR_ODR_13
//#define LED3_PIN		GPIO_ODR_ODR_14
//#define LED4_PIN		GPIO_ODR_ODR_15

//plytka od doktoratu
#define LED_PORT		GPIOE
#define LED1_PIN		GPIO_ODR_ODR_2
#define LED2_PIN		GPIO_ODR_ODR_3
#define LED3_PIN		GPIO_ODR_ODR_4
#define LED4_PIN		GPIO_ODR_ODR_5

#define LED1_ON			LED_PORT->ODR |= LED1_PIN;
#define LED1_OFF		LED_PORT->ODR &= ~LED1_PIN;
#define LED1_TOG		LED_PORT->ODR ^= LED1_PIN;
#define LED2_ON			LED_PORT->ODR |= LED2_PIN;
#define LED2_OFF		LED_PORT->ODR &= ~LED2_PIN;
#define LED2_TOG		LED_PORT->ODR ^= LED2_PIN;
#define LED3_ON			LED_PORT->ODR |= LED3_PIN;
#define LED3_OFF		LED_PORT->ODR &= ~LED3_PIN;
#define LED3_TOG		LED_PORT->ODR ^= LED3_PIN;
#define LED4_ON			LED_PORT->ODR |= LED4_PIN;
#define LED4_OFF		LED_PORT->ODR &= ~LED4_PIN;
#define LED4_TOG		LED_PORT->ODR ^= LED4_PIN;
#define LEDALL_ON		LED_PORT->ODR |= LED1_PIN | LED2_PIN | LED3_PIN | LED4_PIN;
#define LEDALL_OFF	LED_PORT->ODR &= ~LED1_PIN & ~LED2_PIN & ~LED3_PIN & ~LED4_PIN;
#define LEDALL_TOG	LED_PORT->ODR ^= LED1_PIN | LED2_PIN | LED3_PIN | LED4_PIN;

void SystemStart(void);
void delay_ms(uint32_t ms);

#endif
