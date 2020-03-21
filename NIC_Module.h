#ifndef _NIC_MODULE
#define _NIC_MODULE
#include "Control.h"

void NIC_Conf(void);
void NIC_ReadCoils(void);
void NIC_ReadSystemInformation(void);
void NIC_ReadSystemConfiguration(void);
void NIC_ReadNetwrkStatusMb(void);
void NIC_ReadNetwrkConfigurationMb(void);
void NIC_ReadSystemStatusComErrorFlags(void);
void NIC_ReadCommandFlags(void);
void NIC_WriteCoils(void);

#endif
