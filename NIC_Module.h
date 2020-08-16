#ifndef _NIC_MODULE
#define _NIC_MODULE
#include "Control.h"

void NIC_Conf(void);
void NIC_ReadCoils(void);
void NIC_ReadSystemInformation(void);
void NIC_ReadSystemConfiguration(void);
void NIC_ReadSystemStatusComErrorFlags(void);
void NIC_ReadCommandFlags(void);
void NIC_ReadNetworkStatusMb(void);
void NIC_ReadNetworkConfigurationMb(void);
void NIC_ReadNetworkStatusPfbus(void);
void NIC_ReadNetworkConfigurationPfbus(void);
void NIC_ReadNetworkStatusPfnet(void);
void NIC_ReadNetworkConfigurationPfnet(void);
void NIC_WriteCoils(void);
void NIC_WriteSystemConfiguration(void);
void NIC_WriteNetworkConfigurationMb(void);
void NIC_WriteNetworkConfigurationPfbus(void);
void NIC_WriteNetworkConfigurationPfnet(void);
void NIC_WriteCommandFlags(void);
void NIC_WorkTypeRunComunication(void);
void NIC_WorkTypeConfComunication(void);

#endif
