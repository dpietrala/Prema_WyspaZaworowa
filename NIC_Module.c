#include "NIC_Module.h"
extern sControl* pC;
static void NIC_ClrStr(uint8_t* str, uint32_t n)
{
	for(uint32_t i=0;i<n;i++)
		str[i] = 0x00;
}
static uint16_t NIC_Crc16(uint8_t* buf, uint32_t len)
{
  uint16_t crc = 0xffff;
  for (uint32_t pos = 0; pos < len; pos++) 
	{
    crc ^= (uint16_t)buf[pos];
    for (uint8_t i = 8; i != 0; i--)
		{
      if ((crc & 0x0001) != 0) 
			{
        crc >>= 1;
        crc ^= 0xA001;
      }
      else
			{
        crc >>= 1;
			}
    }
  }
  return crc;  
}
void NIC_Conf(void)
{
	DMA1_Stream5->PAR 	= (uint32_t)&USART2->DR;
	DMA1_Stream5->M0AR 	= (uint32_t)pC->Nic.bufread;
	DMA1_Stream5->NDTR 	= (uint16_t)NIC_BUFMAX;
	DMA1_Stream5->CR 		|= DMA_SxCR_MINC | DMA_SxCR_CIRC | DMA_SxCR_CHSEL_2 | DMA_SxCR_EN;
	DMA1_Stream6->PAR 	= (uint32_t)&USART2->DR;
	DMA1_Stream6->M0AR 	= (uint32_t)pC->Nic.bufwrite;
	DMA1_Stream6->NDTR 	= (uint16_t)NIC_BUFMAX;
	DMA1_Stream6->CR 		|= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_DIR_0 | DMA_SxCR_TCIE;
	NVIC_EnableIRQ(DMA1_Stream6_IRQn);
	GPIOA->MODER |= GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR3_0;
	GPIOA->AFR[0] |= 0x00007700;
	USART2->BRR = 25000000/115200;
	USART2->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;
	USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE;
	NVIC_EnableIRQ(USART2_IRQn);
}
static void NIC_BytesToUint8(uint8_t* buf, uint32_t* idx, uint8_t* val)
{
	*val = ((uint16_t)buf[*idx]<<0);
	*idx += 1;
}
static void NIC_BytesToUint16(uint8_t* buf, uint32_t *idx, uint16_t* val)
{
	*val = 0;
	*val += ((uint16_t)buf[*idx+0]<<8);
	*val += ((uint16_t)buf[*idx+1]<<0);
	*idx += 2;
}
static void NIC_BytesToUint32(uint8_t* buf, uint32_t* idx, uint32_t* val)
{
	*val = 0;
	*val += ((uint32_t)buf[*idx+0]<<8);
	*val += ((uint32_t)buf[*idx+1]<<0);
	*val += ((uint32_t)buf[*idx+2]<<24);
	*val += ((uint32_t)buf[*idx+3]<<16);
	*idx += 4;
}
static void NIC_BytesToTableUint8(uint8_t* buf, uint32_t* idx, uint8_t* tab, uint32_t num)
{
	for(int i=0;i<num/2;i++)
	{
		tab[2*i+1] = buf[*idx];
		*idx += 1;
		tab[2*i+0] = buf[*idx];
		*idx += 1;
	}
}
static void NIC_BytesToTableUint16(uint8_t* buf, uint32_t* idx, uint16_t* tab, uint32_t num)
{
	for(int i=0;i<num;i++)
	{
		tab[i] = ((uint16_t)buf[*idx + 0] << 8) + ((uint16_t)buf[*idx + 1] << 0);
		*idx += 2;
	}
}
static void NIC_Uint16ToTableUint16(uint16_t val, uint32_t* idx, uint16_t* tab)
{
	tab[*idx] = val;
	*idx += 1;
}
static void NIC_Uint32ToTableUint16(uint32_t val, uint32_t* idx, uint16_t* tab)
{
	tab[*idx] = val >> 0;
	*idx += 1;
	tab[*idx] = val >> 16;
	*idx += 1;
}
static void NIC_ChangeComStatus(eNicComStatus newstatus)
{
	pC->Nic.mode.comStatus = newstatus;
	pC->Nic.mode.time = 0;
}
static void NIC_SendData(uint8_t* buf, uint32_t num)
{
	NIC_ClrStr(pC->Nic.bufwrite, NIC_BUFMAX);
	for(uint32_t i=0;i<num;i++)
		pC->Nic.bufwrite[i] = buf[i];
	
	DMA1_Stream6->CR &= ~DMA_SxCR_EN;
	DMA1->HIFCR |= DMA_HIFCR_CTCIF6;
	DMA1_Stream6->NDTR 	= num;
	DMA1_Stream6->CR |= DMA_SxCR_EN;
	
	NIC_ChangeComStatus(NCS_comIsSending);
}
static void NIC_ReadRegisters(uint16_t addr0, uint16_t numregs)
{
	uint8_t buf[NIC_BUFMAX];
	uint16_t index = 0;
	buf[index++] = pC->Nic.mode.address;
	buf[index++] = MF_RnHR;
	buf[index++] = addr0 >> 8;
	buf[index++] = addr0 >> 0;
	buf[index++] = numregs >> 8;
	buf[index++] = numregs >> 0;
	
	uint16_t crc = NIC_Crc16(buf, index);
	buf[index++] = crc >> 0;
	buf[index++] = crc >> 8;
	
	NIC_SendData(buf, index);
}
static void NIC_WriteRegs(uint16_t addr0, uint16_t numregs, uint16_t* regs)
{
	uint8_t buf[NIC_BUFMAX];
	
	uint16_t index = 0;
	buf[index++] = pC->Nic.mode.address;
	buf[index++] = MF_WnHR;
	buf[index++] = addr0 >> 8;
	buf[index++] = addr0 >> 0;
	buf[index++] = numregs >> 8;
	buf[index++] = numregs >> 0;
	buf[index++] = 2*numregs;
	for(uint16_t i=0;i<numregs;i++)
	{
		buf[index++] = regs[i] >> 8;
		buf[index++] = regs[i] >> 0;
	}
	uint16_t crc = NIC_Crc16(buf, index);
	buf[index++] = crc >> 0;
	buf[index++] = crc >> 8;
	
	NIC_SendData(buf, index);
}
//**** Funkcje wysylajace do modulu *************************************
void NIC_ReadCoils(void)
{
	NIC_ReadRegisters(1000, 1);
	pC->Nic.mode.nicFun = NF_RC;
}
void NIC_ReadSystemInformation(void)
{
	NIC_ReadRegisters(0, 61); // read only from devNumber to protConfClass
	pC->Nic.mode.nicFun = NF_RSI;
}
void NIC_ReadSystemConfiguration(void)
{
	NIC_ReadRegisters(100, 100);
	pC->Nic.mode.nicFun = NF_RSC;
}
void NIC_ReadSystemStatusComErrorFlags(void)
{
	NIC_ReadRegisters(988, 12);
	pC->Nic.mode.nicFun = NF_RSSCEF;
}
void NIC_ReadCommandFlags(void)
{
	NIC_ReadRegisters(1999, 11);
	pC->Nic.mode.nicFun = NF_RCF;
}
void NIC_ReadNetworkStatusMb(void)
{
	NIC_ReadRegisters(200, 100);
	pC->Nic.mode.nicFun = NF_RNS_MB;
}
void NIC_ReadNetworkConfigurationMb(void)
{
	NIC_ReadRegisters(300, 100);
	pC->Nic.mode.nicFun = NF_RNC_MB;
}
void NIC_ReadNetworkStatusPfbus(void)
{
	NIC_ReadRegisters(200, 100);
	pC->Nic.mode.nicFun = NF_RNS_PFB;
}
void NIC_ReadNetworkConfigurationPfbus(void)
{
	NIC_ReadRegisters(300, 10);
	pC->Nic.mode.nicFun = NF_RNC_PFB;
}
void NIC_ReadNetworkStatusPfnet(void)
{
	NIC_ReadRegisters(200, 100);
	pC->Nic.mode.nicFun = NF_RNS_PFN;
}
void NIC_ReadNetworkConfigurationPfnet(void)
{
	NIC_ReadRegisters(300, 100);
	pC->Nic.mode.nicFun = NF_RNC_PFN;
}
void NIC_WriteCoils(void)
{
	NIC_WriteRegs(2000, 1, &pC->Nic.cod.coils);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteSystemConfiguration(void)
{
	pC->Nic.scWrite = pC->Nic.scRead;
	pC->Nic.scWrite.regs[4] = 5;
	NIC_WriteRegs(100, 100, pC->Nic.scWrite.regs);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationMb(void)
{
	pC->Nic.ncMbWrite = pC->Nic.ncMbRead;
	
	pC->Nic.ncMbWrite.sendAckTimeout = 25000;
	
	uint32_t idx = 12;
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.sendAckTimeout, &idx, pC->Nic.ncMbWrite.regs);
	
	NIC_WriteRegs(300, 100, pC->Nic.ncMbWrite.regs);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationPfbus(void)
{
//	pC->Nic.ncPfbusWrite = pC->Nic.ncPfbusRead;
//	
//	pC->Nic.ncPfbusWrite.sendAckTimeout = 25000;
//	
//	uint32_t idx = 12;
//	NIC_Uint32ToTableUint16(pC->Nic.ncPfbusWrite.sendAckTimeout, &idx, pC->Nic.ncPfbusWrite.regs);
//	
//	NIC_WriteRegs(300, 100, pC->Nic.ncPfbusWrite.regs);
//	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationPfnet(void)
{
//	pC->Nic.ncPfnetWrite = pC->Nic.ncPfnetRead;
//	
//	pC->Nic.ncPfnetWrite.sendAckTimeout = 25000;
//	
//	uint32_t idx = 12;
//	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.sendAckTimeout, &idx, pC->Nic.ncPfnetWrite.regs);
//	
//	NIC_WriteRegs(300, 100, pC->Nic.ncPfnetWrite.regs);
//	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteCommandFlags(void)
{
	uint16_t val = pC->Nic.cfRead.flagsCommand;
	val |= (1<<4);
	NIC_WriteRegs(1999, 1, &val);
	pC->Nic.mode.nicFun = NF_WR;
}
//**** Funkcje czytajace odpowiedzi z modulu *****************************
static void NIC_ReadResponseAfterReadCoils(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	if(address != pC->Nic.mode.address)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		pC->Nic.cid.coils = ((uint16_t)buf[3]<<8) + ((uint16_t)buf[4]<<0);
	}
}
static void NIC_ReadResponseAfterReadSystemInformations(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.address)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToUint32(buf, &idx, &pC->Nic.si.devNumber);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.si.serNumber);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.devClass);
		NIC_BytesToUint8(buf, &idx, &pC->Nic.si.hardRev);
		NIC_BytesToUint8(buf, &idx, &pC->Nic.si.hardCompIndex);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.hardOpChann0);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.hardOpChann1);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.hardOpChann2);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.hardOpChann3);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.si.virtualDPMSize);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.manufCode);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.prodCode);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.ethMACAddr, 6);
		idx += 4; //reserved bytes
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.firm);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.firmVer, 8);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.firmDate, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.firmName, 64);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.comClass);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.protClass);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.protConfClass);
		idx += 18; //reserved bytes
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.inputConfShiftRegs, 10);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.outputStatusShiftRegs, 10);
	}
}
static void NIC_ReadResponseAfterReadSystemConfiguration(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.address)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.ssioType);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.ssioAddress);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.scRead.ssioBaud);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.ssioNumInBytes);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.ssioNumOutBytes);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.shifType);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.shifBaud);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.shifAddress);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.flagsShifConf);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.ssioNumBytesSsioIn);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.ssioNumBytesSsioOut);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.ssioOffsetAddressFbIn);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.ssioOffsetAddressFbOut);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.ssioWatchdogTime);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.ssioSwapShiftDir);
		idx += 8; //reserved bytes
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.offsetAddressOutDataImage);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.numMappData);
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.scRead.mapData, 78);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.scRead.regs, 100);
	}
}
static void NIC_ReadResponseAfterReadSystemStatusComErrorFlags(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.address)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToUint32(buf, &idx, &pC->Nic.sscef.systemStatus);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.sscef.systemError);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.sscef.errorLogInd);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.sscef.errorCounter);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.sscef.comError);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.sscef.comStatus);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.sscef.recPacketSize);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.sscef.flagsSystem);
			
		pC->Nic.sscef.flagReady 				= (eBool)((pC->Nic.sscef.flagsSystem >> 0) & 0x01);
		pC->Nic.sscef.flagError 				= (eBool)((pC->Nic.sscef.flagsSystem >> 1) & 0x01);
		pC->Nic.sscef.flagCommunicating = (eBool)((pC->Nic.sscef.flagsSystem >> 2) & 0x01);
		pC->Nic.sscef.flagNcfError 			= (eBool)((pC->Nic.sscef.flagsSystem >> 3) & 0x01);
		pC->Nic.sscef.flagRxMbxFull 		= (eBool)((pC->Nic.sscef.flagsSystem >> 4) & 0x01);
		pC->Nic.sscef.flagTxMbxFull 		= (eBool)((pC->Nic.sscef.flagsSystem >> 5) & 0x01);
		pC->Nic.sscef.flagBusOn 				= (eBool)((pC->Nic.sscef.flagsSystem >> 6) & 0x01);
		pC->Nic.sscef.flagFlsCfg 				= (eBool)((pC->Nic.sscef.flagsSystem >> 7) & 0x01);
		pC->Nic.sscef.flagLckCfg 				= (eBool)((pC->Nic.sscef.flagsSystem >> 8) & 0x01);
		pC->Nic.sscef.flagWdgOn 				= (eBool)((pC->Nic.sscef.flagsSystem >> 9) & 0x01);
		pC->Nic.sscef.flagRunning 			= (eBool)((pC->Nic.sscef.flagsSystem >> 10) & 0x01);
		pC->Nic.sscef.flagSxWriteInd 		= (eBool)((pC->Nic.sscef.flagsSystem >> 11) & 0x01);
		pC->Nic.sscef.flagRemCfg 				= (eBool)((pC->Nic.sscef.flagsSystem >> 12) & 0x01);
	}
}
static void NIC_ReadResponseAfterReadCommandFlags(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.address)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToUint16(buf, &idx, &pC->Nic.cfRead.flagsCommand);
			
		pC->Nic.cfRead.flagReset 					= (eBool)((pC->Nic.cfRead.flagsCommand >> 0) & 0x01);
		pC->Nic.cfRead.flagBootStart 			= (eBool)((pC->Nic.cfRead.flagsCommand >> 1) & 0x01);
		pC->Nic.cfRead.flagAppReady 			= (eBool)((pC->Nic.cfRead.flagsCommand >> 2) & 0x01);
		pC->Nic.cfRead.flagBusOn 					= (eBool)((pC->Nic.cfRead.flagsCommand >> 3) & 0x01);
		pC->Nic.cfRead.flagInit 					= (eBool)((pC->Nic.cfRead.flagsCommand >> 4) & 0x01);
		pC->Nic.cfRead.flagBusOff 				= (eBool)((pC->Nic.cfRead.flagsCommand >> 5) & 0x01);
		pC->Nic.cfRead.flagClrCfg 				= (eBool)((pC->Nic.cfRead.flagsCommand >> 6) & 0x01);
		pC->Nic.cfRead.flagStrCfg 				= (eBool)((pC->Nic.cfRead.flagsCommand >> 7) & 0x01);
		pC->Nic.cfRead.flagLckCfg 				= (eBool)((pC->Nic.cfRead.flagsCommand >> 8) & 0x01);
		pC->Nic.cfRead.flagUnlockCfg 			= (eBool)((pC->Nic.cfRead.flagsCommand >> 9) & 0x01);
		pC->Nic.cfRead.flagWdgOn 					= (eBool)((pC->Nic.cfRead.flagsCommand >> 10) & 0x01);
		pC->Nic.cfRead.flagWdgOff 				= (eBool)((pC->Nic.cfRead.flagsCommand >> 11) & 0x01);
		pC->Nic.cfRead.flagClrRemCfg 			= (eBool)((pC->Nic.cfRead.flagsCommand >> 12) & 0x01);
	}
}
static void NIC_ReadResponseAfterReadNetworkStatusMb(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.address)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.nsMb.regs, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationMb(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.address)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncMbRead.length);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.busStartup);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.wdgTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.provSerwerConn);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.responseTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.clientConWdgTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.protMode);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.sendAckTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.conAckTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.closeAckTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.dataSwap);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.flagsReg321_322);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncMbRead.ipAddress, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncMbRead.subnetMask, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncMbRead.gateway, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncMbRead.ethAddress, 6);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.flagsReg332_333);
		
		pC->Nic.ncMbRead.responseTimeout *= 100;
		pC->Nic.ncMbRead.clientConWdgTimeout *= 100;
		
		pC->Nic.ncMbRead.flagIpAddressAvailabe 	= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 0) & 0x01);
		pC->Nic.ncMbRead.flagNetMaskAvailabe 		= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 1) & 0x01);
		pC->Nic.ncMbRead.flagGatewayAvailabe 		= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 2) & 0x01);
		pC->Nic.ncMbRead.flagBootIp 						= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 3) & 0x01);
		pC->Nic.ncMbRead.flagDhcp 							= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 4) & 0x01);
		pC->Nic.ncMbRead.flgSetEthAddress 			= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 5) & 0x01);
		
		pC->Nic.ncMbRead.flagMapFc1ToFc3 				= (eBool)((pC->Nic.ncMbRead.flagsReg332_333 >> 0) & 0x01);
		pC->Nic.ncMbRead.flagSkipConfTcpipStack = (eBool)((pC->Nic.ncMbRead.flagsReg332_333 >> 1) & 0x01);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncMbRead.regs, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkStatusPfbus(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.address)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.nsPfbus.regs, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfbus(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.address)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfbusRead.length);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfbusRead.flagsReg301_302);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfbusRead.wdgTimeout);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfbusRead.identNumber);
		NIC_BytesToUint8(buf, &idx, &pC->Nic.ncPfbusRead.baudrate);
		NIC_BytesToUint8(buf, &idx, &pC->Nic.ncPfbusRead.stationAddress);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfbusRead.flagsReg307);
		NIC_BytesToUint8(buf, &idx, &pC->Nic.ncPfbusRead.lengthConfData);
		
		pC->Nic.ncPfbusRead.flagBusStartup 	= (eBool)((pC->Nic.ncPfbusRead.flagsReg301_302 >> 0) & 0x01);
		pC->Nic.ncPfbusRead.flagAddressSwitchEnable 	= (eBool)((pC->Nic.ncPfbusRead.flagsReg301_302 >> 4) & 0x01);
		pC->Nic.ncPfbusRead.flagBaudrateSwitchEnable 	= (eBool)((pC->Nic.ncPfbusRead.flagsReg301_302 >> 5) & 0x01);
		
		pC->Nic.ncPfbusRead.flagDpv1Enable 	= (eBool)((pC->Nic.ncPfbusRead.flagsReg307 >> 0) & 0x01);
		pC->Nic.ncPfbusRead.flagSyncSupperted 	= (eBool)((pC->Nic.ncPfbusRead.flagsReg307 >> 1) & 0x01);
		pC->Nic.ncPfbusRead.flagFreezeSuported 	= (eBool)((pC->Nic.ncPfbusRead.flagsReg307 >> 2) & 0x01);
		pC->Nic.ncPfbusRead.flagFailSafeSuported 	= (eBool)((pC->Nic.ncPfbusRead.flagsReg307 >> 3) & 0x01);
		pC->Nic.ncPfbusRead.flagAlarmSap50Deactivate 	= (eBool)((pC->Nic.ncPfbusRead.flagsReg307 >> 4) & 0x01);
		pC->Nic.ncPfbusRead.flagIoDataSwap 	= (eBool)((pC->Nic.ncPfbusRead.flagsReg307 >> 5) & 0x01);
		pC->Nic.ncPfbusRead.flagAutoConfiguration 	= (eBool)((pC->Nic.ncPfbusRead.flagsReg307 >> 6) & 0x01);
		pC->Nic.ncPfbusRead.flagAddressChangeNotAllowed 	= (eBool)((pC->Nic.ncPfbusRead.flagsReg307 >> 7) & 0x01);
	}
}
static void NIC_ReadResponseAfterReadNetworkStatusPfnet(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.address)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.nsPfnet.regs, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.address)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.length);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.busStartup);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.wdgTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.provSerwerConn);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.responseTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.clientConWdgTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.protMode);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.sendAckTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.conAckTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.closeAckTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.dataSwap);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.flagsReg321_322);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.ipAddress, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.subnetMask, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.gateway, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.ethAddress, 6);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.flagsReg332_333);
		
		pC->Nic.ncPfnetRead.responseTimeout *= 100;
		pC->Nic.ncPfnetRead.clientConWdgTimeout *= 100;
		
		pC->Nic.ncPfnetRead.flagIpAddressAvailabe 	= (eBool)((pC->Nic.ncPfnetRead.flagsReg321_322 >> 0) & 0x01);
		pC->Nic.ncPfnetRead.flagNetMaskAvailabe 		= (eBool)((pC->Nic.ncPfnetRead.flagsReg321_322 >> 1) & 0x01);
		pC->Nic.ncPfnetRead.flagGatewayAvailabe 		= (eBool)((pC->Nic.ncPfnetRead.flagsReg321_322 >> 2) & 0x01);
		pC->Nic.ncPfnetRead.flagBootIp 						= (eBool)((pC->Nic.ncPfnetRead.flagsReg321_322 >> 3) & 0x01);
		pC->Nic.ncPfnetRead.flagDhcp 							= (eBool)((pC->Nic.ncPfnetRead.flagsReg321_322 >> 4) & 0x01);
		pC->Nic.ncPfnetRead.flgSetEthAddress 			= (eBool)((pC->Nic.ncPfnetRead.flagsReg321_322 >> 5) & 0x01);
		
		pC->Nic.ncPfnetRead.flagMapFc1ToFc3 				= (eBool)((pC->Nic.ncPfnetRead.flagsReg332_333 >> 0) & 0x01);
		pC->Nic.ncPfnetRead.flagSkipConfTcpipStack = (eBool)((pC->Nic.ncPfnetRead.flagsReg332_333 >> 1) & 0x01);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.regs, 100);
	}
}
static void NIC_ReadResponseAfterWriteReisters(void)
{
	uint8_t* buf = pC->Nic.bufread;
	if(buf[0] != pC->Nic.mode.address)
		return;
	if(buf[1] != MF_WnHR)
		return;
	uint16_t crc1 = NIC_Crc16(buf, 6);
	uint16_t crc2 = ((uint16_t)buf[6]<<0) + ((uint16_t)buf[7]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		
	}
}
static void NIC_SendNextFunction(void)
{
	pC->Nic.mode.tabFunToSend[pC->Nic.mode.numFunToSend]();
}
uint32_t licz = 0;
void NIC_StartComunication(void)
{
//	if(pC->Nic.mode.time >= pC->Nic.mode.timeout)
//	{
//		pC->Nic.mode.time = pC->Nic.mode.timeout;
//		pC->Nic.mode.errorTimeout = true;
//		pC->Nic.mode.comStatus = NCS_comIsIdle;
//	}
//	else
//	{
//		pC->Nic.mode.errorTimeout = false;
//	}
		licz++;
		pC->Nic.mode.numFunToSend = 0;
		NIC_SendNextFunction();
}
static void NIC_ReadResponse(void)
{
	NIC_ChangeComStatus(NCS_comIsReading);
	uint8_t* buf = pC->Nic.bufread;
	switch(pC->Nic.mode.nicFun)
	{
		case NF_I:
			break;
		case NF_RC:
			NIC_ReadResponseAfterReadCoils();
			break;
		case NF_RSI:
			NIC_ReadResponseAfterReadSystemInformations();
			break;
		case NF_RSC:
			NIC_ReadResponseAfterReadSystemConfiguration();
			break;
		case NF_RSSCEF:
			NIC_ReadResponseAfterReadSystemStatusComErrorFlags();
			break;
		case NF_RCF:
			NIC_ReadResponseAfterReadCommandFlags();
			break;
		case NF_RNS_MB:
			NIC_ReadResponseAfterReadNetworkStatusMb();
			break;
		case NF_RNC_MB:
			NIC_ReadResponseAfterReadNetworkConfigurationMb();
			break;
		case NF_RNS_PFB:
			NIC_ReadResponseAfterReadNetworkStatusPfbus();
			break;
		case NF_RNC_PFB:
			NIC_ReadResponseAfterReadNetworkConfigurationPfbus();
			break;
		case NF_RNS_PFN:
			NIC_ReadResponseAfterReadNetworkStatusPfnet();
			break;
		case NF_RNC_PFN:
			NIC_ReadResponseAfterReadNetworkConfigurationPfnet();
			break;
		case NF_WR:
			NIC_ReadResponseAfterWriteReisters();
			break;
		default:
			break;
	}
	NIC_ClrStr(buf, NIC_BUFMAX);
	DMA1_Stream5->CR &= ~DMA_SxCR_EN;
	DMA1->HIFCR |= DMA_HIFCR_CTCIF5;
	DMA1_Stream5->CR |= DMA_SxCR_EN;
	
	pC->Nic.mode.numFunToSend++;
	if(pC->Nic.mode.numFunToSend >= pC->Nic.mode.maxFunToSend)
	{
		NIC_ChangeComStatus(NCS_comIsDone);
	}
	else
	{
		NIC_SendNextFunction();
	}
}
//***** Interrupts ********************************
void USART2_IRQHandler(void)
{
	if((USART2->SR & USART_SR_IDLE) != RESET)
	{
		NIC_ReadResponse();
		char c = USART2->DR;
	}
}
void DMA1_Stream6_IRQHandler(void)
{
	if((DMA1->HISR & DMA_HISR_TCIF6) != RESET)
	{
		NIC_ChangeComStatus(NCS_comIsWaiting);
		DMA1_Stream6->CR &= ~DMA_SxCR_EN;
		DMA1->HIFCR |= DMA_HIFCR_CTCIF6;
	}
}
