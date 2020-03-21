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
	*val += ((uint32_t)buf[*idx+2]<<24);
	*val += ((uint32_t)buf[*idx+3]<<16);
	*val += ((uint32_t)buf[*idx+0]<<8);
	*val += ((uint32_t)buf[*idx+1]<<0);
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
void NIC_Conf(void)
{
	DMA1_Stream1->PAR 	= (uint32_t)&USART3->DR;
	DMA1_Stream1->M0AR 	= (uint32_t)pC->Nic.bufread;
	DMA1_Stream1->NDTR 	= (uint16_t)NIC_BUFMAX;
	DMA1_Stream1->CR 		|= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_EN;
	DMA1_Stream3->PAR 	= (uint32_t)&USART3->DR;
	DMA1_Stream3->M0AR 	= (uint32_t)pC->Nic.bufwrite;
	DMA1_Stream3->NDTR 	= (uint16_t)NIC_BUFMAX;
	DMA1_Stream3->CR 		|= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_DIR_0;
	GPIOB->MODER |= GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1;
	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR10_0 | GPIO_PUPDR_PUPDR11_0;
	GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR10 | GPIO_OSPEEDER_OSPEEDR11;
	GPIOB->AFR[1] |= 0x00007700;
	USART3->BRR = 42000000/115200;
	USART3->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;
	USART3->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE;
	NVIC_EnableIRQ(USART3_IRQn);
	
	NIC_ClrStr(pC->Nic.bufread, NIC_BUFMAX);
	NIC_ClrStr(pC->Nic.bufwrite, NIC_BUFMAX);
	pC->Nic.nicFun = NF_I;
	pC->Nic.address = 2;
}
static void NIC_ReadRegisters(uint16_t addr0, uint16_t numregs)
{
	uint8_t* buf = pC->Nic.bufwrite;
	NIC_ClrStr(buf, NIC_BUFMAX);
	uint16_t index = 0;
	buf[index++] = pC->Nic.address;
	buf[index++] = MF_RnHR;
	buf[index++] = addr0 >> 8;
	buf[index++] = addr0 >> 0;
	buf[index++] = numregs >> 8;
	buf[index++] = numregs >> 0;
	
	uint16_t crc = NIC_Crc16(buf, index);
	buf[index++] = crc >> 0;
	buf[index++] = crc >> 8;
	
	DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	DMA1_Stream3->NDTR 	= index;
	DMA1_Stream3->CR |= DMA_SxCR_EN;
}
static void NIC_WriteRegs(uint16_t addr0, uint16_t numregs, uint16_t* regs)
{
	uint8_t* buf = pC->Nic.bufwrite;
	NIC_ClrStr(buf, NIC_BUFMAX);
	
	uint16_t index = 0;
	buf[index++] = pC->Nic.address;
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
	
	DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	DMA1_Stream3->NDTR 	= index;
	DMA1_Stream3->CR |= DMA_SxCR_EN;
}
void NIC_ReadCoils(void)
{
	NIC_ReadRegisters(1000, 1);
	pC->Nic.nicFun = NF_RC;
}
void NIC_ReadSystemInformation(void)
{
	NIC_ReadRegisters(0, 61); // read only from devNumber to protConfClass
	pC->Nic.nicFun = NF_RSI;
}
void NIC_ReadSystemConfiguration(void)
{
	NIC_ReadRegisters(100, 100);
	pC->Nic.nicFun = NF_RSC;
}
void NIC_ReadSystemStatusComErrorFlags(void)
{
	NIC_ReadRegisters(988, 12);
	pC->Nic.nicFun = NF_RSSCEF;
}
void NIC_ReadCommandFlags(void)
{
	NIC_ReadRegisters(1999, 11);
	pC->Nic.nicFun = NF_RCF;
}
void NIC_ReadNetwrkStatusMb(void)
{
	NIC_ReadRegisters(200, 100);
	pC->Nic.nicFun = NF_RNS_MB;
}
void NIC_ReadNetwrkConfigurationMb(void)
{
	NIC_ReadRegisters(300, 100);
	pC->Nic.nicFun = NF_RNC_MB;
}
void NIC_WriteCoils(void)
{
	NIC_WriteRegs(2000, 1, &pC->Nic.cod.coils);
	pC->Nic.nicFun = NF_WC;
}
void NIC_WriteSystemConfiguration(void)
{
	pC->Nic.scWrite = pC->Nic.scRead;
	pC->Nic.scWrite.regs[4] = 5;
	NIC_WriteRegs(100, 100, pC->Nic.scWrite.regs);
	pC->Nic.nicFun = NF_WSC;
}
void NIC_WriteNetworkConfiguration(void)
{
	pC->Nic.ncMbWrite = pC->Nic.ncMbRead;
	pC->Nic.ncMbWrite.regs[23] = (0 << 8) + 35;
	NIC_WriteRegs(300, 100, pC->Nic.ncMbWrite.regs);
	pC->Nic.nicFun = NF_WNC;
}
void NIC_WriteCommandFlags(void)
{
	uint16_t val = pC->Nic.cfRead.flagsCommand;
	val |= (1<<4);
	NIC_WriteRegs(1999, 1, &val);
	pC->Nic.nicFun = NF_WCF;
}
static void NIC_ReadRequestAfterReadCoils(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	if(address != pC->Nic.address)
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
static void NIC_ReadRequestAfterReadSystemInformations(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.address)
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
static void NIC_ReadRequestAfterReadSystemConfiguration(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.address)
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
static void NIC_ReadRequestAfterReadSystemStatusComErrorFlags(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.address)
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
static void NIC_ReadRequestAfterReadCommandFlags(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.address)
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
static void NIC_ReadRequestAfterReadNetworkStatusMb(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.address)
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
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.nsMb.ns, 100);
	}
}
static void NIC_ReadRequestAfterReadNetworkConfigurationMb(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.address)
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
static void NIC_ReadRequestAfterWriteCoils(void)
{
	uint8_t* buf = pC->Nic.bufread;
	
	if(buf[0] != pC->Nic.address)
	{
		return;
	}
	if(buf[1] != MF_WnHR)
	{
		return;
	}
	
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
static void NIC_ReadRequestAfterWriteSystemConfiguration(void)
{
	uint8_t* buf = pC->Nic.bufread;
	if(buf[0] != pC->Nic.address)
	{
		return;
	}
	if(buf[1] != MF_WnHR)
	{
		return;
	}
	
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
static void NIC_ReadRequestAfterWriteNetworkConfiguration(void)
{
	uint8_t* buf = pC->Nic.bufread;
	if(buf[0] != pC->Nic.address)
	{
		return;
	}
	if(buf[1] != MF_WnHR)
	{
		return;
	}
	
	uint16_t crc1 = NIC_Crc16(buf, 6);
	uint16_t crc2 = ((uint16_t)buf[6]<<0) + ((uint16_t)buf[7]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		LED4_TOG;
	}
}
static void NIC_ReadRequestAfterWriteCommandFlags(void)
{
	uint8_t* buf = pC->Nic.bufread;
	
	if(buf[0] != pC->Nic.address)
	{
		return;
	}
	if(buf[1] != MF_WnHR)
	{
		return;
	}
	
	uint16_t crc1 = NIC_Crc16(buf, 6);
	uint16_t crc2 = ((uint16_t)buf[6]<<0) + ((uint16_t)buf[7]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		LED2_TOG;
	}
}
static void NIC_ReadReaquest(void)
{
	LED3_OFF;
	uint8_t* buf = pC->Nic.bufread;
	switch(pC->Nic.nicFun)
	{
		case NF_I:
			break;
		case NF_RC:
			NIC_ReadRequestAfterReadCoils();
			break;
		case NF_RSI:
			NIC_ReadRequestAfterReadSystemInformations();
			break;
		case NF_RSC:
			NIC_ReadRequestAfterReadSystemConfiguration();
			break;
		case NF_RSSCEF:
			NIC_ReadRequestAfterReadSystemStatusComErrorFlags();
			break;
		case NF_RCF:
			NIC_ReadRequestAfterReadCommandFlags();
			break;
		case NF_RNS_MB:
			NIC_ReadRequestAfterReadNetworkStatusMb();
			break;
		case NF_RNC_MB:
			NIC_ReadRequestAfterReadNetworkConfigurationMb();
			break;
		case NF_WC:
			NIC_ReadRequestAfterWriteCoils();
			break;
		case NF_WSC:
			NIC_ReadRequestAfterWriteSystemConfiguration();
			break;
		case NF_WNC:
			NIC_ReadRequestAfterWriteNetworkConfiguration();
			break;
		case NF_WCF:
			NIC_ReadRequestAfterWriteCommandFlags();
			break;
		default:
			LED3_ON;
			break;
	}
	NIC_ClrStr(buf, NIC_BUFMAX);
	DMA1_Stream1->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF1;
	DMA1_Stream1->CR |= DMA_SxCR_EN;
}
void USART3_IRQHandler(void)
{
	if((USART3->SR & USART_SR_IDLE) != RESET)
	{
		NIC_ReadReaquest();
		char c = USART3->DR;
	}
}





//static void NIC_ReadRequestAfterWriteRegs(void)
//{
//	uint8_t* buf = pC->Nic.bufread;
//	uint8_t address = buf[0];
//	uint8_t flag = 0;
//	
//	if(address == pC->Nic.address)
//		flag = 1;
//	if(flag == 0)
//		return;
//	if(buf[1] != pC->Nic.fun)
//		return;
//	
//	uint16_t crc1 = NIC_Crc16(buf, 6);
//	uint16_t crc2 = ((uint16_t)buf[6]<<0) + ((uint16_t)buf[7]<<8);
//	if(crc1 != crc2)
//	{
//		return;
//	}
//	else
//	{
//		pC->Nic.fun = MF_I;
//		pC->Nic.reg0 = 0;
//		pC->Nic.numregs = 0;
//	}
//	return;
//}
//static void NIC_ReadRequestAfterFunctionError(void)
//{
//	uint8_t* buf = pC->Nic.bufread;
//	uint8_t address = buf[0];
//	uint8_t fun = buf[1];
//	uint8_t bytes = buf[2];
//	uint8_t regs = bytes/2;
//	uint8_t flag = 0;
//	for(uint8_t i=0;i<SLAVEMAX;i++)
//		if(address == pC->Nic.slaves[i].address)
//			flag = 1;
//	if(flag == 0)
//		return;
//	
//	if(fun != pC->Nic.slaves[address].state)
//		return;

//	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
//	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
//	if(crc1 != crc2)
//	{
//		return;
//	}
//	else
//	{
//		LED3_TOG;
//		for(uint8_t i=0;i<regs;i++)
//		{
//			uint16_t reg = ((uint16_t)buf[3+2*i+0]<<8) + ((uint16_t)buf[3+2*i+1]<<0);
//			pC->Nic.slaves[address].holdreg[pC->Nic.slaves[address].addr0 + i] = reg;
//		}
//		pC->Nic.slaves[address].state = MFI;
//		pC->Nic.slaves[address].addr0 = 0;
//		pC->Nic.slaves[address].numregs = 0;
//	}
//	return;
//}
