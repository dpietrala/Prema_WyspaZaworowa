#include "Control.h"
sControl Control;
sControl* pC = &Control;
int main(void)
{
	SystemStart();
	Led_Conf();
	NIC_Conf();
  while(1)
  {
		LED1_TOG;
		//NIC_ReadCoils();
		//NIC_ReadSystemInformation();
		//NIC_ReadSystemConfiguration();
		//NIC_ReadNetwrkStatusMb();
		NIC_ReadNetwrkConfigurationMb();
		delay_ms(100);
  }
}
