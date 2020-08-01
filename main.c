#include "Control.h"
sControl Control;
sControl* pC = &Control;
uint8_t zmienna1 = 0;
int main(void)
{
	Control_SystemStart();
	Outputs_Conf();
//	NIC_Conf();
  while(1)
  {
		LED1_TOG;
		zmienna1++;
		
		OUT0_OFF;
		OUT1_OFF;
		OUT2_OFF;
		OUT3_OFF;
		OUT4_OFF;
		OUT5_OFF;
		OUT6_OFF;
		OUT7_OFF;
		OUT8_OFF;
		OUT9_OFF;
		OUT10_OFF;
		OUT11_OFF;
		OUT12_OFF;
		OUT13_OFF;
		OUT14_OFF;
		OUT15_OFF;
		delay_ms(1000);
		
		LED1_TOG;
		zmienna1++;
		OUT0_ON;
		OUT1_ON;
		OUT2_ON;
		OUT3_ON;
		OUT4_ON;
		OUT5_ON;
		OUT6_ON;
		OUT7_ON;
		OUT8_ON;
		OUT9_ON;
		OUT10_ON;
		OUT11_ON;
		OUT12_ON;
		OUT13_ON;
		OUT14_ON;
		OUT15_ON;
		delay_ms(1000);
  }
}
