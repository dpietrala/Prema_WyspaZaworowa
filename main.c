#include "Control.h"
sControl Control;
sControl* pC = &Control;
int main(void)
{
	Control_SystemInit();
	Outputs_Conf();
	NIC_Conf();
	delay_ms(3000);
	Control_WorkTypeConf();
	Control_SystemStart();
  while(1)
  {
		
	}
}
