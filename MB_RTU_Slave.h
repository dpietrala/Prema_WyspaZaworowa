#ifndef _MB_RTU_SLAVE
#define _MB_RTU_SLAVE
#include "Control.h"
#include <math.h>

void MBS_Conf(void);
void MBS_ClrStr(uint8_t*, uint32_t);
uint16_t MBS_Crc16(uint8_t*, uint32_t);

#endif
