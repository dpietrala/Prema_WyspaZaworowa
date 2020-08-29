#ifndef _NIC_MODULE
#define _NIC_MODULE
#include "Control.h"

void NIC_BytesToUint8(uint8_t* buf, uint32_t* idx, uint8_t* val);
void NIC_BytesToUint16(uint8_t* buf, uint32_t *idx, uint16_t* val);
void NIC_BytesToUint32(uint8_t* buf, uint32_t* idx, uint32_t* val);
void NIC_BytesToTableUint8(uint8_t* bufsource, uint32_t* idxsource, uint8_t* tabdest, uint32_t startidxdest, uint32_t num);
void NIC_BytesToTableUint16(uint8_t* buf, uint32_t* sidx, uint16_t* tab, uint32_t startdestidx, uint32_t num);
void NIC_TableUint8ToTableUint16(uint8_t* source, uint16_t* dest, uint32_t* destidx, uint32_t num);
void NIC_TableUint16ToTableUint16(uint16_t* source, uint16_t* dest, uint32_t* destidx, uint32_t num);
void NIC_Uint8ToTableUint16(uint8_t highbyte, uint8_t lowbyte, uint32_t* idx, uint16_t* tab);
void NIC_Uint16ToTableUint16(uint16_t val, uint32_t* idx, uint16_t* tab);
void NIC_Uint32ToTableUint16(uint32_t val, uint32_t* idx, uint16_t* tab);

eResult NIC_ComConf(void);
void NIC_ReadCoils(void);
void NIC_ReadSystemInformation(void);
void NIC_ReadSystemConfiguration(void);
void NIC_ReadSystemStatusComErrorFlags(void);
void NIC_ReadCommandFlags(void);
void NIC_ReadNetworkStatusMb(void);
void NIC_ReadNetworkConfigurationMb300_333(void);
void NIC_ReadNetworkStatusPfbus(void);
void NIC_ReadNetworkConfigurationPfbus300_399(void);
void NIC_ReadNetworkConfigurationPfbus400_430(void);
void NIC_ReadNetworkStatusPfnet(void);
void NIC_ReadNetworkConfigurationPfnet300_399(void);
void NIC_ReadNetworkConfigurationPfnet400_499(void);
void NIC_ReadNetworkConfigurationPfnet500_598(void);
void NIC_ReadNetworkConfigurationPfnet599_699(void);
void NIC_ReadNetworkConfigurationPfnet700_799(void);
void NIC_ReadNetworkConfigurationPfnet800_899(void);
void NIC_ReadNetworkConfigurationPfnet900_987(void);
void NIC_WriteStatusMb(void);
void NIC_WriteStatusPfbus(void);
void NIC_WriteStatusPfnet(void);
void NIC_WriteSystemConfigurationBaudrate(void);
void NIC_WriteSystemConfiguration(void);
void NIC_WriteNetworkConfigurationMb300_333(void);
void NIC_WriteNetworkConfigurationPfbus300_399(void);
void NIC_WriteNetworkConfigurationPfbus400_430(void);
void NIC_WriteNetworkConfigurationPfnet300_399(void);
void NIC_WriteNetworkConfigurationPfnet400_499(void);
void NIC_WriteNetworkConfigurationPfnet500_598(void);
void NIC_WriteNetworkConfigurationPfnet599_699(void);
void NIC_WriteNetworkConfigurationPfnet700_799(void);
void NIC_WriteNetworkConfigurationPfnet800_899(void);
void NIC_WriteNetworkConfigurationPfnet900_987(void);
void NIC_WriteClrcfgFlagInCommandFlags(void);
void NIC_WriteStrcfgFlagInCommandFlags(void);
void NIC_WriteInitFlagInCommandFlags(void);
void NIC_WriteCommandFlags(void);
void NIC_StartComunication(uint8_t num, uint32_t timeout);
void NIC_ConverseConfiguratonToRegsMb(void);
void NIC_SetDefaultSystemConfiguration(void);
void NIC_SetDefaultSystemInformationMb(void);
void NIC_SetDefaultConfigurationMb(void);
void NIC_SetDefaultSystemInformationPfbus(void);
void NIC_SetDefaultConfigurationPfbus(void);
void NIC_SetDefaultSystemInformationPfnet(void);
void NIC_SetDefaultConfigurationPfnet(void);


#endif
