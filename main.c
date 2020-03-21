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
		NIC_ReadCoils();
		delay_ms(100);
//		NIC_ReadSystemInformation();
//		delay_ms(100);
//		NIC_ReadSystemConfiguration();
//		delay_ms(100);
//		NIC_ReadSystemStatusComErrorFlags();
//		delay_ms(100);
//		NIC_ReadNetwrkStatusMb();
//		delay_ms(100);
//		NIC_ReadNetwrkConfigurationMb();
//		delay_ms(100);
//		NIC_ReadCommandFlags();
//		delay_ms(50);
//		NIC_WriteCoils();
//		delay_ms(25);
//		NIC_WriteNetworkConfiguration();
//		delay_ms(500);
//		NIC_WriteCommandFlags();
//		delay_ms(500);
  }
}
