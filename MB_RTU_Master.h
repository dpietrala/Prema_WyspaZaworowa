#ifndef _MB_RTU_MASTER
#define _MB_RTU_MASTER
#include "Control.h"

void MBM_Conf(void);
void MBM_ClrStr(uint8_t*, uint32_t);
uint16_t MBM_Crc16(uint8_t*, uint32_t);
void MBM_WriteRegs(uint16_t, uint16_t);
void MBM_ReadRegs(uint16_t, uint16_t);

#endif
