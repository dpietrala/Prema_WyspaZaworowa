#include "Outputs.h"
extern sControl* pC;
void Outputs_Conf(void)
{
	
}
static void Outputs_ChangeState(void)
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
void Outputs_WorkTypeStop(void)
{
	pC->Outs.coils = 0x00;
	Outputs_ChangeState();
}
void Outputs_WorkTypeRun(void)
{
	if(pC->Mode.protocol == Prot_Mbrtu)
		pC->Outs.coils = pC->Mbs.coils;
	else if(pC->Mode.protocol == Prot_Mbtcp)
		pC->Outs.coils = pC->Nic.cid.coils;
	else if(pC->Mode.protocol == Prot_Pfbus)
		pC->Outs.coils = pC->Nic.cid.coils;
	else if(pC->Mode.protocol == Prot_Pfnet)
		pC->Outs.coils = pC->Nic.cid.coils;
	
	Outputs_ChangeState();
}
void Outputs_WorkTypeConfiguration(void)
{
	pC->Outs.coils = 0x00;
	Outputs_ChangeState();
}
void Outputs_WorkTypeError(void)
{
	pC->Outs.coils = 0x00;
	Outputs_ChangeState();
}
