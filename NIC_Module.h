#ifndef _NIC_MODULE
#define _NIC_MODULE
#include "Control.h"

void NIC_Conf(void);
void NIC_ClrStr(uint8_t*, uint32_t);
uint16_t NIC_Crc16(uint8_t*, uint32_t);
void NIC_WriteRegs(uint16_t, uint16_t);
void NIC_ReadRegs(uint16_t, uint16_t);

#endif
