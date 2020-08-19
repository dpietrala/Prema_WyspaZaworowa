#ifndef _NIC_MODULE
#define _NIC_MODULE
#include "Control.h"

void NIC_BytesToUint8(uint8_t* buf, uint32_t* idx, uint8_t* val);
void NIC_BytesToUint16(uint8_t* buf, uint32_t *idx, uint16_t* val);
void NIC_BytesToUint32(uint8_t* buf, uint32_t* idx, uint32_t* val);
void NIC_BytesToTableUint8(uint8_t* buf, uint32_t* idx, uint8_t* tab, uint32_t num);
void NIC_BytesToTableUint16(uint8_t* buf, uint32_t* idx, uint16_t* tab, uint32_t num);
void NIC_TableUint8ToTableUint16(uint8_t* source, uint16_t* dest, uint32_t* destidx, uint32_t num);
void NIC_Uint16ToTableUint16(uint16_t val, uint32_t* idx, uint16_t* tab);
void NIC_Uint32ToTableUint16(uint32_t val, uint32_t* idx, uint16_t* tab);


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
void NIC_WriteClrcfgFlagInCommandFlags(void);
void NIC_WriteStrcfgFlagInCommandFlags(void);
void NIC_WriteInitFlagInCommandFlags(void);
void NIC_WriteCommandFlags(void);
void NIC_StartComunication(uint8_t num, uint32_t timeout);
void NIC_ConverseConfiguratonToRegsMb(void);
void NIC_SetDefaultSystemInformationMb(void);
void NIC_SetDefaultConfigurationMb(void);
void NIC_SetDefaultSystemInformationPfnet(void);
void NIC_SetDefaultConfigurationPfnet(void);


#endif
