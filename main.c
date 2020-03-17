#include "Control.h"
sControl Control;
sControl* pC = &Control;
int main(void)
{
	SystemStart();
	Led_Conf();
	MBM_Conf();
  while(1)
  {
		LED1_TOG;
		delay_ms(100);
  }
}
