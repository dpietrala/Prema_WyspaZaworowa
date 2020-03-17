#include "Outputs.h"
extern sControl* pC;
void Led_Conf(void)
{
//	LED_PORT->MODER 	|= GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;
//	LED_PORT->PUPDR 	|= GPIO_PUPDR_PUPDR12_0 | GPIO_PUPDR_PUPDR13_0 | GPIO_PUPDR_PUPDR14_0 | GPIO_PUPDR_PUPDR15_0;
	
	LED_PORT->MODER 	|= GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0;
	LED_PORT->PUPDR 	|= GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR3_0 | GPIO_PUPDR_PUPDR4_0 | GPIO_PUPDR_PUPDR5_0;
}
void Outputs_Conf(void)
{
	
}
void Outputs_ChangeState(void)
{
	uint16_t temp;
	temp = pC->Outs.coils >> 0;
	if((temp & 0x01) == RESET)		OUT0_OFF;
	else													OUT0_ON;
	temp = pC->Outs.coils >> 1;
	if((temp & 0x01) == RESET)		OUT1_OFF;
	else													OUT1_ON;
	temp = pC->Outs.coils >> 2;
	if((temp & 0x01) == RESET)		OUT2_OFF;
	else													OUT2_ON;
	temp = pC->Outs.coils >> 3;
	if((temp & 0x01) == RESET)		OUT3_OFF;
	else													OUT3_ON;
	temp = pC->Outs.coils >> 4;
	if((temp & 0x01) == RESET)		OUT4_OFF;
	else													OUT4_ON;
	temp = pC->Outs.coils >> 5;
	if((temp & 0x01) == RESET)		OUT5_OFF;
	else													OUT5_ON;
	temp = pC->Outs.coils >> 6;
	if((temp & 0x01) == RESET)		OUT6_OFF;
	else													OUT6_ON;
	temp = pC->Outs.coils >> 7;
	if((temp & 0x01) == RESET)		OUT7_OFF;
	else													OUT7_ON;
	
	temp = pC->Outs.coils >> 8;
	if((temp & 0x01) == RESET)		OUT8_OFF;
	else													OUT8_ON;
	temp = pC->Outs.coils >> 9;
	if((temp & 0x01) == RESET)		OUT9_OFF;
	else													OUT9_ON;
	temp = pC->Outs.coils >> 10;
	if((temp & 0x01) == RESET)		OUT10_OFF;
	else													OUT10_ON;
	temp = pC->Outs.coils >> 11;
	if((temp & 0x01) == RESET)		OUT11_OFF;
	else													OUT11_ON;
	temp = pC->Outs.coils >> 12;
	if((temp & 0x01) == RESET)		OUT12_OFF;
	else													OUT12_ON;
	temp = pC->Outs.coils >> 13;
	if((temp & 0x01) == RESET)		OUT13_OFF;
	else													OUT13_ON;
	temp = pC->Outs.coils >> 14;
	if((temp & 0x01) == RESET)		OUT14_OFF;
	else													OUT14_ON;
	temp = pC->Outs.coils >> 15;
	if((temp & 0x01) == RESET)		OUT15_OFF;
	else													OUT15_ON;
}
