#ifndef _OUTPUTS
#define _OUTPUTS
#include "Control.h"
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


void Outputs_Conf(void);
void Outputs_ChangeState(void);


#endif
