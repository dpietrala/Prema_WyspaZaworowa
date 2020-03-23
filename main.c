#include "Control.h"
sControl Control;
sControl* pC = &Control;
int main(void)
{
	Control_SystemStart();
	NIC_Conf();
  while(1)
  {
		LED1_TOG;
		delay_ms(100);
  }
}
