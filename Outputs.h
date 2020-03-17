#ifndef _OUTPUTS
#define _OUTPUTS
#include "Control.h"
//******************************************************************************************
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

//*****************************************************************************************
#define OUT0_ON			GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT0_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT1_ON			GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT1_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT2_ON			GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT2_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT3_ON			GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT3_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT4_ON			GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT4_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT5_ON			GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT5_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT6_ON			GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT6_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT7_ON			GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT7_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT8_ON			GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT8_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT9_ON			GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT9_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT10_ON		GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT10_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT11_ON		GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT11_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT12_ON		GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT12_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT13_ON		GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT13_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT14_ON		GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT14_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0
#define OUT15_ON		GPIOE->ODR |= GPIO_ODR_ODR_0
#define OUT15_OFF		GPIOE->ODR &= ~GPIO_ODR_ODR_0


void Led_Conf(void);
void Outputs_Conf(void);
void Outputs_ChangeState(void);


#endif
