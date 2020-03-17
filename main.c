#include "Control.h"
extern sModbus* pModbus;
int main(void)
{
	SystemStart();
	Modbus_Conf();
  while(1)
  {
		LED1_TOG;
		delay_ms(100);
  }
}
