#include "Control.h"
sControl Control;
sControl* pC = &Control;
int main(void)
{
	Control_SystemInit();
	Outputs_Conf();
	NIC_Conf();
	Control_SystemStart();
  while(1)
  {
		
	}
}
