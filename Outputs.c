#include "Outputs.h"
extern sControl* pC;
eResult Outputs_Conf(void)
{
	eResult result = RES_OK;
	
	OUT0_PORT->MODER &= ~OUT0_MODERALL;
	OUT0_PORT->MODER |= OUT0_MODER;
	OUT0_PORT->PUPDR |= OUT0_PUPDR;
	OUT0_OFF;
	
	OUT1_PORT->MODER &= ~OUT1_MODERALL;
	OUT1_PORT->MODER |= OUT1_MODER;
	OUT1_PORT->PUPDR |= OUT1_PUPDR;
	OUT1_OFF;
	
	OUT2_PORT->MODER &= ~OUT2_MODERALL;
	OUT2_PORT->MODER |= OUT2_MODER;
	OUT2_PORT->PUPDR |= OUT2_PUPDR;
	OUT2_OFF;
	
	OUT3_PORT->MODER &= ~OUT3_MODERALL;
	OUT3_PORT->MODER |= OUT3_MODER;
	OUT3_PORT->PUPDR |= OUT3_PUPDR;
	OUT3_OFF;
	
	OUT4_PORT->MODER &= ~OUT4_MODERALL;
	OUT4_PORT->MODER |= OUT4_MODER;
	OUT4_PORT->PUPDR |= OUT4_PUPDR;
	OUT4_OFF;
	
	OUT5_PORT->MODER &= ~OUT5_MODERALL;
	OUT5_PORT->MODER |= OUT5_MODER;
	OUT5_PORT->PUPDR |= OUT5_PUPDR;
	OUT5_OFF;
	
	OUT6_PORT->MODER &= ~OUT6_MODERALL;
	OUT6_PORT->MODER |= OUT6_MODER;
	OUT6_PORT->PUPDR |= OUT6_PUPDR;
	OUT6_OFF;
	
	OUT7_PORT->MODER &= ~OUT7_MODERALL;
	OUT7_PORT->MODER |= OUT7_MODER;
	OUT7_PORT->PUPDR |= OUT7_PUPDR;
	OUT7_OFF;
	
	OUT8_PORT->MODER &= ~OUT8_MODERALL;
	OUT8_PORT->MODER |= OUT8_MODER;
	OUT8_PORT->PUPDR |= OUT8_PUPDR;
	OUT8_OFF;
	
	OUT9_PORT->MODER &= ~OUT9_MODERALL;
	OUT9_PORT->MODER |= OUT9_MODER;
	OUT9_PORT->PUPDR |= OUT9_PUPDR;
	OUT9_OFF;

	OUT10_PORT->MODER &= ~OUT10_MODERALL;
	OUT10_PORT->MODER |= OUT10_MODER;
	OUT10_PORT->PUPDR |= OUT10_PUPDR;
	OUT10_OFF;
	
	OUT11_PORT->MODER &= ~OUT11_MODERALL;
	OUT11_PORT->MODER |= OUT11_MODER;
	OUT11_PORT->PUPDR |= OUT11_PUPDR;
	OUT11_OFF;
	
	OUT12_PORT->MODER &= ~OUT12_MODERALL;
	OUT12_PORT->MODER |= OUT12_MODER;
	OUT12_PORT->PUPDR |= OUT12_PUPDR;
	OUT12_OFF;
	
	OUT13_PORT->MODER &= ~OUT13_MODERALL;
	OUT13_PORT->MODER |= OUT13_MODER;
	OUT13_PORT->PUPDR |= OUT13_PUPDR;
	OUT13_OFF;
	
	OUT14_PORT->MODER &= ~OUT14_MODERALL;
	OUT14_PORT->MODER |= OUT14_MODER;
	OUT14_PORT->PUPDR |= OUT14_PUPDR;
	OUT14_OFF;
	
	OUT15_PORT->MODER &= ~OUT15_MODERALL;
	OUT15_PORT->MODER |= OUT15_MODER;
	OUT15_PORT->PUPDR |= OUT15_PUPDR;
	OUT15_OFF;
	
	
	OUT0_PORT->OTYPER |= OUT0_OTYPER;
	OUT1_PORT->OTYPER |= OUT1_OTYPER;
	OUT2_PORT->OTYPER |= OUT2_OTYPER;
	OUT3_PORT->OTYPER |= OUT3_OTYPER;
	OUT4_PORT->OTYPER |= OUT4_OTYPER;
	OUT5_PORT->OTYPER |= OUT5_OTYPER;
	OUT6_PORT->OTYPER |= OUT6_OTYPER;
	OUT7_PORT->OTYPER |= OUT7_OTYPER;
	OUT8_PORT->OTYPER |= OUT8_OTYPER;
	OUT9_PORT->OTYPER |= OUT9_OTYPER;
	OUT10_PORT->OTYPER |= OUT10_OTYPER;
	OUT11_PORT->OTYPER |= OUT11_OTYPER;
	OUT12_PORT->OTYPER |= OUT12_OTYPER;
	OUT13_PORT->OTYPER |= OUT13_OTYPER;
	OUT14_PORT->OTYPER |= OUT14_OTYPER;
	OUT15_PORT->OTYPER |= OUT15_OTYPER;
	
	return result;
}
static void Outputs_ChangeState(void)
{
	uint16_t temp;
	uint16_t tab[16];
	for(uint16_t i=0;i<16;i++)
	{
		temp = pC->Outs.coilsReq >> i;
		tab[i] = (temp & 0x01);
	}
	for(uint16_t i=0;i<8;i++)
	{
		if(tab[i] == 1)
			tab[i + 8] = 0;
	}
	pC->Outs.coils = 0;
	for(uint16_t i=0;i<16;i++)
	{
		pC->Outs.coils += tab[i] << i;
	}
	
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
void Outputs_RunModbusRTU(void)
{
}
void Outputs_RunModbusTCP(void)
{
	if(pC->Status.flagBusMbtcpCommunicating && pC->Status.flagBusMbtcpRunning)
	{
		pC->Outs.coilsReq = pC->Nic.cid.coils;
	}
	Outputs_ChangeState();
}
void Outputs_RunProfiBUS(void)
{
	if(pC->Status.flagBusPfbusCommunicating && pC->Status.flagBusPfbusRunning)
	{
		pC->Outs.coilsReq = pC->Nic.cid.coils;
	}
	Outputs_ChangeState();
}
void Outputs_RunProfiNET(void)
{
	if(pC->Status.flagBusPfnetCommunicating && pC->Status.flagBusPfnetRunning)
	{
		pC->Outs.coilsReq = pC->Nic.cid.coils;
	}
	Outputs_ChangeState();
}
