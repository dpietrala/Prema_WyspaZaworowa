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
eResult NIC_ComConf(void)
{
	eResult result = RES_OK;
	
	DMA1_Stream5->PAR 	= (uint32_t)&USART2->DR;
	DMA1_Stream5->M0AR 	= (uint32_t)pC->Nic.bufread;
	DMA1_Stream5->NDTR 	= (uint16_t)NIC_BUFMAX;
	DMA1_Stream5->CR 		= DMA_SxCR_MINC | DMA_SxCR_CIRC | DMA_SxCR_CHSEL_2 | DMA_SxCR_EN;
	DMA1_Stream6->PAR 	= (uint32_t)&USART2->DR;
	DMA1_Stream6->M0AR 	= (uint32_t)pC->Nic.bufwrite;
	DMA1_Stream6->NDTR 	= (uint16_t)NIC_BUFMAX;
	DMA1_Stream6->CR 		= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_DIR_0 | DMA_SxCR_TCIE;
	NVIC_EnableIRQ(DMA1_Stream6_IRQn);
	
	GPIOA->MODER |= GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR3_0;
	GPIOA->AFR[0] |= 0x00007700;
	
	USART2->BRR = 25000000/pC->Nic.mode.comBaud;
	USART2->CR3 = USART_CR3_DMAR | USART_CR3_DMAT;
	USART2->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_UE | USART_CR1_IDLEIE;
	NVIC_EnableIRQ(USART2_IRQn);
	return result;
}
void NIC_BytesToUint8(uint8_t* buf, uint32_t* idx, uint8_t* val)
{
	*val = ((uint16_t)buf[*idx]<<0);
	*idx += 1;
}
void NIC_BytesToUint16(uint8_t* buf, uint32_t *idx, uint16_t* val)
{
	*val = 0;
	*val += ((uint16_t)buf[*idx+0]<<8);
	*val += ((uint16_t)buf[*idx+1]<<0);
	*idx += 2;
}
void NIC_BytesToUint32(uint8_t* buf, uint32_t* idx, uint32_t* val)
{
	*val = 0;
	*val += ((uint32_t)buf[*idx+0]<<8);
	*val += ((uint32_t)buf[*idx+1]<<0);
	*val += ((uint32_t)buf[*idx+2]<<24);
	*val += ((uint32_t)buf[*idx+3]<<16);
	*idx += 4;
}
void NIC_BytesToTableUint8(uint8_t* bufsource, uint32_t* idxsource, uint8_t* tabdest, uint32_t startidxdest, uint32_t num)
{
	for(int i=0;i<num/2;i++)
	{
		tabdest[2*i+1 + startidxdest] = bufsource[*idxsource];
		*idxsource += 1;
		tabdest[2*i+0 + startidxdest] = bufsource[*idxsource];
		*idxsource += 1;
	}
}
void NIC_BytesToTableUint16(uint8_t* buf, uint32_t* sidx, uint16_t* tab, uint32_t startdestidx, uint32_t num)
{
	for(int i=0;i<num;i++)
	{
		tab[i + startdestidx] = ((uint16_t)buf[*sidx + 0] << 8) + ((uint16_t)buf[*sidx + 1] << 0);
		*sidx += 2;
	}
}
void NIC_TableUint8ToTableUint16(uint8_t* source, uint16_t* dest, uint32_t* destidx, uint32_t num)
{
	for(int i=0;i<num/2;i++)
	{
		dest[*destidx] = ((uint16_t)source[2*i+1] << 8) + ((uint16_t)source[2*i+0] << 0);
		*destidx += 1;
	}
}
void NIC_TableUint16ToTableUint16(uint16_t* source, uint16_t* dest, uint32_t* destidx, uint32_t num)
{
	for(int i=0;i<num;i++)
	{
		dest[*destidx] = source[i];
		*destidx += 1;
	}
}
void NIC_Uint8ToTableUint16(uint8_t highbyte, uint8_t lowbyte, uint32_t* idx, uint16_t* tab)
{
	tab[*idx] = ((uint16_t)highbyte << 8) + ((uint16_t)lowbyte << 0);
	*idx += 1;
}
void NIC_Uint16ToTableUint16(uint16_t val, uint32_t* idx, uint16_t* tab)
{
	tab[*idx] = val;
	*idx += 1;
}
void NIC_Uint32ToTableUint16(uint32_t val, uint32_t* idx, uint16_t* tab)
{
	tab[*idx] = val >> 0;
	*idx += 1;
	tab[*idx] = val >> 16;
	*idx += 1;
}
static void NIC_ChangeComStatus(eNicComStatus newstatus)
{
	pC->Nic.mode.comStatus = newstatus;
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
	buf[index++] = pC->Nic.mode.comAddress;
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
static void NIC_WriteRegs(uint16_t addr0, uint16_t numregs, uint16_t* regs, uint32_t startnumreg)
{
	uint8_t buf[NIC_BUFMAX];
	
	uint16_t index = 0;
	buf[index++] = pC->Nic.mode.comAddress;
	buf[index++] = MF_WnHR;
	buf[index++] = addr0 >> 8;
	buf[index++] = addr0 >> 0;
	buf[index++] = numregs >> 8;
	buf[index++] = numregs >> 0;
	buf[index++] = 2*numregs;
	for(uint16_t i=0;i<numregs;i++)
	{
		buf[index++] = regs[i + startnumreg] >> 8;
		buf[index++] = regs[i + startnumreg] >> 0;
	}
	uint16_t crc = NIC_Crc16(buf, index);
	buf[index++] = crc >> 0;
	buf[index++] = crc >> 8;
	
	NIC_SendData(buf, index);
}
void NIC_SetDefaultSystemConfiguration(void)
{
	pC->Nic.scDef.ssioType = 0;
	pC->Nic.scDef.ssioAddress = 0;
	pC->Nic.scDef.ssioBaud = 100000;
	pC->Nic.scDef.ssioNumInBytes = 0;
	pC->Nic.scDef.ssioNumOutBytes = 0;
	pC->Nic.scDef.shifType = 0;
	pC->Nic.scDef.shifBaud = 1152;
	pC->Nic.scDef.shifAddress = 2;
	pC->Nic.scDef.flagsShifConf = 16;
	pC->Nic.scDef.ssioNumBytesSsioIn = 0;
	pC->Nic.scDef.ssioNumBytesSsioOut = 0;
	pC->Nic.scDef.ssioOffsetAddressFbIn = 0;
	pC->Nic.scDef.ssioOffsetAddressFbOut = 0;
	pC->Nic.scDef.ssioWatchdogTime = 1000;
	pC->Nic.scDef.ssioSwapShiftDir = 0;
	for(uint16_t i=0;i<4;i++)
		pC->Nic.scDef.reserved[i] = 0;
	pC->Nic.scDef.offsetAddressOutDataImage = 0;
	pC->Nic.scDef.numMappData = 0;
	for(uint16_t i=0;i<78;i++)
		pC->Nic.scDef.mapData[i] = 0;
	
	uint32_t idx = 0;
	NIC_Uint16ToTableUint16(pC->Nic.scDef.ssioType, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.ssioAddress, &idx, pC->Nic.scDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.scDef.ssioBaud, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.ssioNumInBytes, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.ssioNumOutBytes, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.shifType, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.shifBaud, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.shifAddress, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.flagsShifConf, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.ssioNumBytesSsioIn, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.ssioNumBytesSsioOut, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.ssioOffsetAddressFbIn, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.ssioOffsetAddressFbOut, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.ssioWatchdogTime, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.ssioSwapShiftDir, &idx, pC->Nic.scDef.regs);
	NIC_TableUint16ToTableUint16(pC->Nic.scDef.reserved, pC->Nic.scDef.regs, &idx, 4);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.numMappData, &idx, pC->Nic.scDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scDef.offsetAddressOutDataImage, &idx, pC->Nic.scDef.regs);
	NIC_TableUint16ToTableUint16(pC->Nic.scDef.mapData, pC->Nic.scDef.regs, &idx, 78);
}
void NIC_SetDefaultSystemInformationMb(void)
{
	uint32_t tabidx = 0;
	
	pC->Nic.siDefMb.devNumber = 1541100;
	pC->Nic.siDefMb.serNumber = 73180;			//doesnt matter
	pC->Nic.siDefMb.devClass = 19;
	pC->Nic.siDefMb.hardRev = 1;						//doesnt matter
	pC->Nic.siDefMb.hardCompIndex = 0;			//doesnt matter
	pC->Nic.siDefMb.hardOpChann0 = 128;			//doesnt matter
	pC->Nic.siDefMb.hardOpChann1 = 128;			//doesnt matter
	pC->Nic.siDefMb.hardOpChann2 = 65534;		//doesnt matter
	pC->Nic.siDefMb.hardOpChann3 = 65534;		//doesnt matter
	pC->Nic.siDefMb.virtualDPMSize = 65536;	//doesnt matter
	pC->Nic.siDefMb.manufCode = 1;					//doesnt matter
	pC->Nic.siDefMb.prodDate = 4896;				//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefMb.ethMACAddr[tabidx++] = 0x00; 	//doesnt matter
	pC->Nic.siDefMb.ethMACAddr[tabidx++] = 0x02; 	//doesnt matter
	pC->Nic.siDefMb.ethMACAddr[tabidx++] = 0xa2; 	//doesnt matter
	pC->Nic.siDefMb.ethMACAddr[tabidx++] = 0x57; 	//doesnt matter
	pC->Nic.siDefMb.ethMACAddr[tabidx++] = 0x2d; 	//doesnt matter
	pC->Nic.siDefMb.ethMACAddr[tabidx++] = 0x46; 	//doesnt matter
	pC->Nic.siDefMb.firm = 0;											//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefMb.firmVer[tabidx++] = 1; 	//doesnt matter
	pC->Nic.siDefMb.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefMb.firmVer[tabidx++] = 2; 	//doesnt matter
	pC->Nic.siDefMb.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefMb.firmVer[tabidx++] = 16; //doesnt matter
	pC->Nic.siDefMb.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefMb.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefMb.firmVer[tabidx++] = 0; 	//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefMb.firmDate[tabidx++] = 255;	//doesnt matter
	pC->Nic.siDefMb.firmDate[tabidx++] = 7;		//doesnt matter
	pC->Nic.siDefMb.firmDate[tabidx++] = 6;		//doesnt matter
	pC->Nic.siDefMb.firmDate[tabidx++] = 31;	//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 9;
	pC->Nic.siDefMb.firmName[tabidx++] = 'M';
	pC->Nic.siDefMb.firmName[tabidx++] = 'o';
	pC->Nic.siDefMb.firmName[tabidx++] = 'd';
	pC->Nic.siDefMb.firmName[tabidx++] = 'b';
	pC->Nic.siDefMb.firmName[tabidx++] = 'u';
	pC->Nic.siDefMb.firmName[tabidx++] = 's';
	pC->Nic.siDefMb.firmName[tabidx++] = 'T';
	pC->Nic.siDefMb.firmName[tabidx++] = 'C';
	pC->Nic.siDefMb.firmName[tabidx++] = 'P';
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.firmName[tabidx++] = 0;
	pC->Nic.siDefMb.comClass = 6;						//doesnt matter
	pC->Nic.siDefMb.protClass = 18;
	pC->Nic.siDefMb.protConfClass = 0;			//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefMb.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefMb.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefMb.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
}
void NIC_SetDefaultConfigurationMb(void)
{
	pC->Nic.ncMbDef.length = 66;	//66 Bayts
	pC->Nic.ncMbDef.busStartup = 0;	//Automatic
	pC->Nic.ncMbDef.wdgTimeout = 1000;
	pC->Nic.ncMbDef.provSerwerConn = 16;
	pC->Nic.ncMbDef.responseTimeout = 2000;	//2000ms
	pC->Nic.ncMbDef.clientConWdgTimeout = 1000; //1000ms
	pC->Nic.ncMbDef.protMode = 1; //Server mode
	pC->Nic.ncMbDef.sendAckTimeout = 31000; //31000ms
	pC->Nic.ncMbDef.conAckTimeout = 31000; //31000ms
	pC->Nic.ncMbDef.closeAckTimeout = 13000; //13000ms
	pC->Nic.ncMbDef.dataSwap = 0; //Data will not be swapped
	pC->Nic.ncMbDef.flagsReg321_322 = 0x00000007; //IP address available, Netmask available, Gateway available
	pC->Nic.ncMbDef.flagIpAddressAvailabe = true; //IP address available,
	pC->Nic.ncMbDef.flagNetMaskAvailabe = true; //Netmask available,
	pC->Nic.ncMbDef.flagGatewayAvailabe = true;	//Gateway available
	pC->Nic.ncMbDef.flagBootIp = false;
	pC->Nic.ncMbDef.flagDhcp = false;
	pC->Nic.ncMbDef.flagSetEthAddress = false;
	pC->Nic.ncMbDef.ipAddress[0] = 19;
	pC->Nic.ncMbDef.ipAddress[1] = 0;
	pC->Nic.ncMbDef.ipAddress[2] = 168;
	pC->Nic.ncMbDef.ipAddress[3] = 192;
	pC->Nic.ncMbDef.subnetMask[0] = 0;
	pC->Nic.ncMbDef.subnetMask[1] = 255;
	pC->Nic.ncMbDef.subnetMask[2] = 255;
	pC->Nic.ncMbDef.subnetMask[3] = 255;
	pC->Nic.ncMbDef.gateway[0] = 1;
	pC->Nic.ncMbDef.gateway[1] = 0;
	pC->Nic.ncMbDef.gateway[2] = 168;
	pC->Nic.ncMbDef.gateway[3] = 192;
	pC->Nic.ncMbDef.ethAddress[0] = 0;
	pC->Nic.ncMbDef.ethAddress[1] = 0;
	pC->Nic.ncMbDef.ethAddress[2] = 0;
	pC->Nic.ncMbDef.ethAddress[3] = 0;
	pC->Nic.ncMbDef.ethAddress[4] = 0;
	pC->Nic.ncMbDef.ethAddress[5] = 0;
	pC->Nic.ncMbDef.flagsReg332_333 = 0x00000000;
	pC->Nic.ncMbDef.flagMapFc1ToFc3 = false;
	pC->Nic.ncMbDef.flagSkipConfTcpipStack = false;
	
	uint32_t idx = 0;
	NIC_Uint16ToTableUint16(pC->Nic.ncMbDef.length, &idx, pC->Nic.ncMbDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.busStartup, &idx, pC->Nic.ncMbDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.wdgTimeout, &idx, pC->Nic.ncMbDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.provSerwerConn, &idx, pC->Nic.ncMbDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.responseTimeout/100, &idx, pC->Nic.ncMbDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.clientConWdgTimeout/100, &idx, pC->Nic.ncMbDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.protMode, &idx, pC->Nic.ncMbDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.sendAckTimeout, &idx, pC->Nic.ncMbDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.conAckTimeout, &idx, pC->Nic.ncMbDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.closeAckTimeout, &idx, pC->Nic.ncMbDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.dataSwap, &idx, pC->Nic.ncMbDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.flagsReg321_322, &idx, pC->Nic.ncMbDef.regs);
	NIC_TableUint8ToTableUint16(pC->Nic.ncMbDef.ipAddress, pC->Nic.ncMbDef.regs, &idx, 4);
	NIC_TableUint8ToTableUint16(pC->Nic.ncMbDef.subnetMask, pC->Nic.ncMbDef.regs, &idx, 4);
	NIC_TableUint8ToTableUint16(pC->Nic.ncMbDef.gateway, pC->Nic.ncMbDef.regs, &idx, 4);
	NIC_TableUint8ToTableUint16(pC->Nic.ncMbDef.ethAddress, pC->Nic.ncMbDef.regs, &idx, 6);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbDef.flagsReg332_333, &idx, pC->Nic.ncMbDef.regs);
}
void NIC_SetDefaultSystemInformationPfbus(void)
{
	uint32_t tabidx = 0;
	
	pC->Nic.siDefPfbus.devNumber = 1541420;
	pC->Nic.siDefPfbus.serNumber = 30581;				//doesnt matter
	pC->Nic.siDefPfbus.devClass = 19;
	pC->Nic.siDefPfbus.hardRev = 0;							//doesnt matter
	pC->Nic.siDefPfbus.hardCompIndex = 2;				//doesnt matter
	pC->Nic.siDefPfbus.hardOpChann0 = 80;				//doesnt matter
	pC->Nic.siDefPfbus.hardOpChann1 = 65534;		//doesnt matter
	pC->Nic.siDefPfbus.hardOpChann2 = 65534;		//doesnt matter
	pC->Nic.siDefPfbus.hardOpChann3 = 65534;		//doesnt matter
	pC->Nic.siDefPfbus.virtualDPMSize = 65536;	//doesnt matter
	pC->Nic.siDefPfbus.manufCode = 1;						//doesnt matter
	pC->Nic.siDefPfbus.prodDate = 4907;					//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfbus.ethMACAddr[tabidx++] = 0x00; //doesnt matter
	pC->Nic.siDefPfbus.ethMACAddr[tabidx++] = 0x00; //doesnt matter
	pC->Nic.siDefPfbus.ethMACAddr[tabidx++] = 0x00; //doesnt matter
	pC->Nic.siDefPfbus.ethMACAddr[tabidx++] = 0x00; //doesnt matter
	pC->Nic.siDefPfbus.ethMACAddr[tabidx++] = 0x00; //doesnt matter
	pC->Nic.siDefPfbus.ethMACAddr[tabidx++] = 0x00; //doesnt matter
	pC->Nic.siDefPfbus.firm = 0;										//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfbus.firmVer[tabidx++] = 1; 	//doesnt matter
	pC->Nic.siDefPfbus.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefPfbus.firmVer[tabidx++] = 5; 	//doesnt matter
	pC->Nic.siDefPfbus.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefPfbus.firmVer[tabidx++] = 16; //doesnt matter
	pC->Nic.siDefPfbus.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefPfbus.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefPfbus.firmVer[tabidx++] = 0; 	//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfbus.firmDate[tabidx++] = 255;	//doesnt matter
	pC->Nic.siDefPfbus.firmDate[tabidx++] = 7;		//doesnt matter
	pC->Nic.siDefPfbus.firmDate[tabidx++] = 6;		//doesnt matter
	pC->Nic.siDefPfbus.firmDate[tabidx++] = 31;	//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 14;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'P';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'R';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'O';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'F';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'I';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'B';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'U';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'S';
	pC->Nic.siDefPfbus.firmName[tabidx++] = ' ';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'S';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'l';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'a';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'v';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 'e';
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.firmName[tabidx++] = 0;
	pC->Nic.siDefPfbus.comClass = 3;						//doesnt matter
	pC->Nic.siDefPfbus.protClass = 19;
	pC->Nic.siDefPfbus.protConfClass = 0;			//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfbus.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfbus.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfbus.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
}
void NIC_SetDefaultConfigurationPfbus(void)
{
	for(uint16_t i=0;i<PFBUS_REGMAX;i++)
		pC->Nic.ncPfbusDef.regs[i] = 0x0000;
	
	pC->Nic.ncPfbusDef.flagBusStartup = false;
	pC->Nic.ncPfbusDef.flagAddressSwitchEnable = false;
	pC->Nic.ncPfbusDef.flagBaudrateSwitchEnable = false;
	
	pC->Nic.ncPfbusDef.flagsReg301_302 = 0x00000000;
	pC->Nic.ncPfbusDef.flagsReg301_302 += (pC->Nic.ncPfbusDef.flagBusStartup << 0);
	pC->Nic.ncPfbusDef.flagsReg301_302 += (pC->Nic.ncPfbusDef.flagAddressSwitchEnable << 4);
	pC->Nic.ncPfbusDef.flagsReg301_302 += (pC->Nic.ncPfbusDef.flagBaudrateSwitchEnable << 5);
	
	pC->Nic.ncPfbusDef.wdgTimeout = 1000;
	pC->Nic.ncPfbusDef.identNumber = 0x0C10;
	pC->Nic.ncPfbusDef.stationAddress = 3;
	pC->Nic.ncPfbusDef.baudrate = 15;
	
	pC->Nic.ncPfbusDef.flagDpv1Enable = true;
	pC->Nic.ncPfbusDef.flagSyncSupperted = true;
	pC->Nic.ncPfbusDef.flagFreezeSuported = true;
	pC->Nic.ncPfbusDef.flagFailSafeSuported = true;
	pC->Nic.ncPfbusDef.flagAlarmSap50Deactivate = false;
	pC->Nic.ncPfbusDef.flagIoDataSwap = false;
	pC->Nic.ncPfbusDef.flagAutoConfiguration = false;
	pC->Nic.ncPfbusDef.flagAddressChangeNotAllowed = true;
	pC->Nic.ncPfbusDef.flagsReg307 = 0x0000;
	pC->Nic.ncPfbusDef.flagsReg307 += (pC->Nic.ncPfbusDef.flagDpv1Enable << 0);
	pC->Nic.ncPfbusDef.flagsReg307 += (pC->Nic.ncPfbusDef.flagSyncSupperted << 1);
	pC->Nic.ncPfbusDef.flagsReg307 += (pC->Nic.ncPfbusDef.flagFreezeSuported << 2);
	pC->Nic.ncPfbusDef.flagsReg307 += (pC->Nic.ncPfbusDef.flagFailSafeSuported << 3);
	pC->Nic.ncPfbusDef.flagsReg307 += (pC->Nic.ncPfbusDef.flagAlarmSap50Deactivate << 4);
	pC->Nic.ncPfbusDef.flagsReg307 += (pC->Nic.ncPfbusDef.flagIoDataSwap << 5);
	pC->Nic.ncPfbusDef.flagsReg307 += (pC->Nic.ncPfbusDef.flagAutoConfiguration << 6);
	pC->Nic.ncPfbusDef.flagsReg307 += (pC->Nic.ncPfbusDef.flagAddressChangeNotAllowed << 7);
	
	pC->Nic.ncPfbusDef.lengthConfData = 4;
	pC->Nic.ncPfbusDef.confData[0] = 0xD0E0;	//Input0, Output0
	pC->Nic.ncPfbusDef.confData[1] = 0xD0D0;	//Input3, Input1
	
	pC->Nic.ncPfbusDef.length = 16 + pC->Nic.ncPfbusDef.lengthConfData;

	uint32_t idx = 0;
	NIC_Uint16ToTableUint16(pC->Nic.ncPfbusDef.length, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfbusDef.flagsReg301_302, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfbusDef.wdgTimeout, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfbusDef.identNumber, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint8ToTableUint16(pC->Nic.ncPfbusDef.baudrate, pC->Nic.ncPfbusDef.stationAddress, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfbusDef.flagsReg307, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint8ToTableUint16(pC->Nic.ncPfbusDef.lengthConfData, 0x00, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_TableUint16ToTableUint16(pC->Nic.ncPfbusDef.confData, pC->Nic.ncPfbusDef.regs, &idx, pC->Nic.ncPfbusDef.lengthConfData/2);
	
}
void NIC_SetDefaultSystemInformationPfnet(void)
{
	uint32_t tabidx = 0;
	
	pC->Nic.siDefPfnet.devNumber = 1541100;
	pC->Nic.siDefPfnet.serNumber = 73180;			//doesnt matter
	pC->Nic.siDefPfnet.devClass = 19;
	pC->Nic.siDefPfnet.hardRev = 1;						//doesnt matter
	pC->Nic.siDefPfnet.hardCompIndex = 4;			//doesnt matter
	pC->Nic.siDefPfnet.hardOpChann0 = 128;			//doesnt matter
	pC->Nic.siDefPfnet.hardOpChann1 = 128;			//doesnt matter
	pC->Nic.siDefPfnet.hardOpChann2 = 65534;		//doesnt matter
	pC->Nic.siDefPfnet.hardOpChann3 = 65534;		//doesnt matter
	pC->Nic.siDefPfnet.virtualDPMSize = 65536;	//doesnt matter
	pC->Nic.siDefPfnet.manufCode = 1;					//doesnt matter
	pC->Nic.siDefPfnet.prodDate = 4896;				//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfnet.ethMACAddr[tabidx++] = 0x00; //doesnt matter
	pC->Nic.siDefPfnet.ethMACAddr[tabidx++] = 0x02; //doesnt matter
	pC->Nic.siDefPfnet.ethMACAddr[tabidx++] = 0xa2; //doesnt matter
	pC->Nic.siDefPfnet.ethMACAddr[tabidx++] = 0x57; //doesnt matter
	pC->Nic.siDefPfnet.ethMACAddr[tabidx++] = 0x2d; //doesnt matter
	pC->Nic.siDefPfnet.ethMACAddr[tabidx++] = 0x46; //doesnt matter
	
	pC->Nic.siDefPfnet.firm = 0;										//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfnet.firmVer[tabidx++] = 1; 	//doesnt matter
	pC->Nic.siDefPfnet.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefPfnet.firmVer[tabidx++] = 5; 	//doesnt matter
	pC->Nic.siDefPfnet.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefPfnet.firmVer[tabidx++] = 17; 	//doesnt matter
	pC->Nic.siDefPfnet.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefPfnet.firmVer[tabidx++] = 0; 	//doesnt matter
	pC->Nic.siDefPfnet.firmVer[tabidx++] = 0; 	//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfnet.firmDate[tabidx++] = 255;	//doesnt matter
	pC->Nic.siDefPfnet.firmDate[tabidx++] = 7;		//doesnt matter
	pC->Nic.siDefPfnet.firmDate[tabidx++] = 11;		//doesnt matter
	pC->Nic.siDefPfnet.firmDate[tabidx++] = 2;	//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 14;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'P';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'R';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'O';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'F';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'I';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'N';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'E';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'T';
	pC->Nic.siDefPfnet.firmName[tabidx++] = ' ';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'S';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'l';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'a';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'v';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 'e';
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.firmName[tabidx++] = 0;
	pC->Nic.siDefPfnet.comClass = 10;					//doesnt matter
	pC->Nic.siDefPfnet.protClass = 21;
	pC->Nic.siDefPfnet.protConfClass = 67;			//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfnet.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.inputConfShiftRegs[tabidx++] = 0;//doesnt matter
	
	tabidx = 0;
	pC->Nic.siDefPfnet.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
	pC->Nic.siDefPfnet.outputStatusShiftRegs[tabidx++] = 0;//doesnt matter
}
void NIC_SetDefaultConfigurationPfnet(void)
{
	pC->Nic.ncPfnetDef.length = 924;
	pC->Nic.ncPfnetDef.busStartup = 0x0300;
	pC->Nic.ncPfnetDef.wdgTimeout = 1000;
	pC->Nic.ncPfnetDef.vendorId = 0x011E;
	pC->Nic.ncPfnetDef.deviceId = 0x010A;
	pC->Nic.ncPfnetDef.maxAR = 0;
	pC->Nic.ncPfnetDef.inputBytes = 4;
	pC->Nic.ncPfnetDef.outputBytes = 10;
	
	pC->Nic.ncPfnetDef.lengthNameOfStation = 10;
	pC->Nic.ncPfnetDef.nameOfStation[0] = 'n';
	pC->Nic.ncPfnetDef.nameOfStation[1] = 'i';
	pC->Nic.ncPfnetDef.nameOfStation[2] = 'c';
	pC->Nic.ncPfnetDef.nameOfStation[3] = '5';
	pC->Nic.ncPfnetDef.nameOfStation[4] = '0';
	pC->Nic.ncPfnetDef.nameOfStation[5] = 'r';
	pC->Nic.ncPfnetDef.nameOfStation[6] = 'e';
	pC->Nic.ncPfnetDef.nameOfStation[7] = 'p';
	pC->Nic.ncPfnetDef.nameOfStation[8] = 'n';
	pC->Nic.ncPfnetDef.nameOfStation[9] = 's';
	for(uint16_t i=pC->Nic.ncPfnetDef.lengthNameOfStation;i<240;i++)
		pC->Nic.ncPfnetDef.nameOfStation[i] = 0;
	
	pC->Nic.ncPfnetDef.lengthTypeOfStation = 19;
	pC->Nic.ncPfnetDef.typeOfStation[0] = 'D';
	pC->Nic.ncPfnetDef.typeOfStation[1] = 'e';
	pC->Nic.ncPfnetDef.typeOfStation[2] = 'f';
	pC->Nic.ncPfnetDef.typeOfStation[3] = 'a';
	pC->Nic.ncPfnetDef.typeOfStation[4] = 'u';
	pC->Nic.ncPfnetDef.typeOfStation[5] = 'l';
	pC->Nic.ncPfnetDef.typeOfStation[6] = 't';
	pC->Nic.ncPfnetDef.typeOfStation[7] = '.';
	pC->Nic.ncPfnetDef.typeOfStation[8] = 'D';
	pC->Nic.ncPfnetDef.typeOfStation[9] = 'e';
	pC->Nic.ncPfnetDef.typeOfStation[10] = 'v';
	pC->Nic.ncPfnetDef.typeOfStation[11] = 'i';
	pC->Nic.ncPfnetDef.typeOfStation[12] = 'c';
	pC->Nic.ncPfnetDef.typeOfStation[13] = 'e';
	pC->Nic.ncPfnetDef.typeOfStation[14] = '.';
	pC->Nic.ncPfnetDef.typeOfStation[15] = 'T';
	pC->Nic.ncPfnetDef.typeOfStation[16] = 'y';
	pC->Nic.ncPfnetDef.typeOfStation[17] = 'p';
	pC->Nic.ncPfnetDef.typeOfStation[18] = 'e';
	for(uint16_t i=pC->Nic.ncPfnetDef.lengthTypeOfStation;i<240;i++)
		pC->Nic.ncPfnetDef.typeOfStation[i] = 0;
		
	pC->Nic.ncPfnetDef.deviceType[0] = 'D';
	pC->Nic.ncPfnetDef.deviceType[1] = 'e';
	pC->Nic.ncPfnetDef.deviceType[2] = 'f';
	pC->Nic.ncPfnetDef.deviceType[3] = 'a';
	pC->Nic.ncPfnetDef.deviceType[4] = 'u';
	pC->Nic.ncPfnetDef.deviceType[5] = 'l';
	pC->Nic.ncPfnetDef.deviceType[6] = 't';
	pC->Nic.ncPfnetDef.deviceType[7] = '.';
	pC->Nic.ncPfnetDef.deviceType[8] = 'D';
	pC->Nic.ncPfnetDef.deviceType[9] = 'e';
	pC->Nic.ncPfnetDef.deviceType[10] = 'v';
	pC->Nic.ncPfnetDef.deviceType[11] = 'i';
	pC->Nic.ncPfnetDef.deviceType[12] = 'c';
	pC->Nic.ncPfnetDef.deviceType[13] = 'e';
	pC->Nic.ncPfnetDef.deviceType[14] = '.';
	pC->Nic.ncPfnetDef.deviceType[15] = 'T';
	pC->Nic.ncPfnetDef.deviceType[16] = 'y';
	pC->Nic.ncPfnetDef.deviceType[17] = 'p';
	pC->Nic.ncPfnetDef.deviceType[18] = 'e';
	pC->Nic.ncPfnetDef.deviceType[20] = 0;
	pC->Nic.ncPfnetDef.deviceType[21] = 0;
	pC->Nic.ncPfnetDef.deviceType[22] = 0;
	pC->Nic.ncPfnetDef.deviceType[23] = 0;
	pC->Nic.ncPfnetDef.deviceType[24] = 0;
	pC->Nic.ncPfnetDef.deviceType[25] = 0;
	pC->Nic.ncPfnetDef.deviceType[26] = 0;
	pC->Nic.ncPfnetDef.deviceType[27] = 0;
	
	pC->Nic.ncPfnetDef.orderId[0] = '1';
	pC->Nic.ncPfnetDef.orderId[1] = '5';
	pC->Nic.ncPfnetDef.orderId[2] = '4';
	pC->Nic.ncPfnetDef.orderId[3] = '1';
	pC->Nic.ncPfnetDef.orderId[4] = '.';
	pC->Nic.ncPfnetDef.orderId[5] = '1';
	pC->Nic.ncPfnetDef.orderId[6] = '0';
	pC->Nic.ncPfnetDef.orderId[7] = '0';
	pC->Nic.ncPfnetDef.orderId[8] = 0;
	pC->Nic.ncPfnetDef.orderId[9] = 0;
	pC->Nic.ncPfnetDef.orderId[10] = 0;
	pC->Nic.ncPfnetDef.orderId[11] = 0;
	pC->Nic.ncPfnetDef.orderId[12] = 0;
	pC->Nic.ncPfnetDef.orderId[13] = 0;
	pC->Nic.ncPfnetDef.orderId[14] = 0;
	pC->Nic.ncPfnetDef.orderId[15] = 0;
	pC->Nic.ncPfnetDef.orderId[16] = 0;
	pC->Nic.ncPfnetDef.orderId[17] = 0;
	pC->Nic.ncPfnetDef.orderId[18] = 0;
	pC->Nic.ncPfnetDef.orderId[19] = 0;
	
	pC->Nic.ncPfnetDef.ipAddress[0] = 19;
	pC->Nic.ncPfnetDef.ipAddress[1] = 0;
	pC->Nic.ncPfnetDef.ipAddress[2] = 168;
	pC->Nic.ncPfnetDef.ipAddress[3] = 192;
	pC->Nic.ncPfnetDef.subnetMask[0] = 0;
	pC->Nic.ncPfnetDef.subnetMask[1] = 255;
	pC->Nic.ncPfnetDef.subnetMask[2] = 255;
	pC->Nic.ncPfnetDef.subnetMask[3] = 255;
	pC->Nic.ncPfnetDef.gateway[0] = 1;
	pC->Nic.ncPfnetDef.gateway[1] = 0;
	pC->Nic.ncPfnetDef.gateway[2] = 168;
	pC->Nic.ncPfnetDef.gateway[3] = 192;
	
	pC->Nic.ncPfnetDef.hardwareRevision = 1;
	pC->Nic.ncPfnetDef.softwareRevision1 = 1;
	pC->Nic.ncPfnetDef.softwareRevision2 = 5;
	pC->Nic.ncPfnetDef.softwareRevision3 = 0;
	pC->Nic.ncPfnetDef.softwareRevisionPrefix = ((uint16_t)'R' << 0);
	
	pC->Nic.ncPfnetDef.maximumDiagRecords = 256;
	pC->Nic.ncPfnetDef.instanceId = 1;
	pC->Nic.ncPfnetDef.reserved1 = 0;
	
	pC->Nic.ncPfnetDef.numApi = 1;
	pC->Nic.ncPfnetDef.profileApi = 0;
	pC->Nic.ncPfnetDef.numSubmoduleItem = 8;
	pC->Nic.ncPfnetDef.slot = 0;
	pC->Nic.ncPfnetDef.subslot = 1;
	pC->Nic.ncPfnetDef.moduleId = 0x0101;
	pC->Nic.ncPfnetDef.subModuleId = 0x0100;
	pC->Nic.ncPfnetDef.provDataLen = 0;
	pC->Nic.ncPfnetDef.consDataLen = 0;
	pC->Nic.ncPfnetDef.dpmOffsetIn = 0xFFFFFFFF;
	pC->Nic.ncPfnetDef.dpmOffsetOut = 0xFFFFFFFF;
	pC->Nic.ncPfnetDef.offsetIopsProvider = 0;
	pC->Nic.ncPfnetDef.offsetIopsConsumer = 0;
	
	pC->Nic.ncPfnetDef.reserved2[0] = 0;
	pC->Nic.ncPfnetDef.reserved2[1] = 0;
	pC->Nic.ncPfnetDef.reserved2[2] = 0;
	pC->Nic.ncPfnetDef.reserved2[3] = 0;
	
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[0] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[1] = 0x8000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[2] = 0x0101;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[3] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[4] = 0x0101;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[5] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[6] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[7] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[8] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[9] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[10] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[11] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[12] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[13] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[14] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[15] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[16] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[17] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[18] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[19] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[20] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[21] = 0x8001;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[22] = 0x0101;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[23] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[24] = 0x0102;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[25] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[26] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[27] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[28] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[29] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[30] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[31] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[32] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[33] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[34] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[35] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[36] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[37] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[38] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[39] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[40] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[41] = 0x8002;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[42] = 0x0101;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[43] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[44] = 0x0103;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[45] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[46] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[47] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[48] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[49] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[50] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[51] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[52] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[53] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[54] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[55] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[56] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[57] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[58] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[59] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[60] = 0x0001;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[61] = 0x0001;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[62] = 0x0035;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[63] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[64] = 0x0034;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[65] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[66] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[67] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[68] = 0x0002;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[69] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[70] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[71] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[72] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[73] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[74] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[75] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[76] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[77] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[78] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[79] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[80] = 0x0005;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[81] = 0x0001;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[82] = 0x0034;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[83] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[84] = 0x0033;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[85] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[86] = 0x0002;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[87] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[88] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[89] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[90] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[91] = 0xFFFF;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[92] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[93] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[94] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[95] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[96] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[97] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[98] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[99] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[100] = 0x0006;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[101] = 0x0001;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[102] = 0x0034;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[103] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[104] = 0x0033;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[105] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[106] = 0x0002;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[107] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[108] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[109] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[110] = 0xffff;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[111] = 0xffff;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[112] = 0x0002;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[113] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[114] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[115] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[116] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[117] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[118] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[119] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[120] = 0x0007;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[121] = 0x0001;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[122] = 0x0034;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[123] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[124] = 0x0033;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[125] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[126] = 0x0002;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[127] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[128] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[129] = 0x0000;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[130] = 0xffff;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[131] = 0xffff;
	pC->Nic.ncPfnetDef.structureSubmoduleOrApi[132] = 0x0004;
	
	
	for(uint32_t i=133;i<365;i++)
		pC->Nic.ncPfnetDef.structureSubmoduleOrApi[i] = 0x0000;


	uint32_t idx = 0;
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.length, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.busStartup, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.wdgTimeout, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.vendorId, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.deviceId, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.maxAR, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.inputBytes, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.outputBytes, &idx, pC->Nic.ncPfnetDef.regs);
	
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.lengthNameOfStation, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetDef.nameOfStation, pC->Nic.ncPfnetDef.regs, &idx, 240);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.lengthTypeOfStation, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetDef.typeOfStation, pC->Nic.ncPfnetDef.regs, &idx, 240);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetDef.deviceType, pC->Nic.ncPfnetDef.regs, &idx, 28);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetDef.orderId, pC->Nic.ncPfnetDef.regs, &idx, 20);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetDef.ipAddress, pC->Nic.ncPfnetDef.regs, &idx, 4);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetDef.subnetMask, pC->Nic.ncPfnetDef.regs, &idx, 4);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetDef.gateway, pC->Nic.ncPfnetDef.regs, &idx, 4);
	
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.hardwareRevision, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.softwareRevision1, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.softwareRevision2, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.softwareRevision3, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.softwareRevisionPrefix, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.maximumDiagRecords, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.instanceId, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.reserved1, &idx, pC->Nic.ncPfnetDef.regs);
	
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.numApi, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.profileApi, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.numSubmoduleItem, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.slot, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.subslot, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.moduleId, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.subModuleId, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.provDataLen, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.consDataLen, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.dpmOffsetIn, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetDef.dpmOffsetOut, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.offsetIopsProvider, &idx, pC->Nic.ncPfnetDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetDef.offsetIopsConsumer, &idx, pC->Nic.ncPfnetDef.regs);
	
	NIC_TableUint16ToTableUint16(pC->Nic.ncPfnetDef.reserved2, pC->Nic.ncPfnetDef.regs, &idx, 4);
	NIC_TableUint16ToTableUint16(pC->Nic.ncPfnetDef.structureSubmoduleOrApi, pC->Nic.ncPfnetDef.regs, &idx, 365);
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
	NIC_ReadRegisters(100, SC_REGMAX);
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
void NIC_ReadNetworkConfigurationMb300_333(void)
{
	NIC_ReadRegisters(300, 34);
	pC->Nic.mode.nicFun = NF_RNC_MB_300_333;
}
void NIC_ReadNetworkStatusPfbus(void)
{
	NIC_ReadRegisters(200, 100);
	pC->Nic.mode.nicFun = NF_RNS_PFB;
}
void NIC_ReadNetworkConfigurationPfbus300_399(void)
{
	NIC_ReadRegisters(300, 100);
	pC->Nic.mode.nicFun = NF_RNC_PFB_300_399;
}
void NIC_ReadNetworkConfigurationPfbus400_430(void)
{
	NIC_ReadRegisters(400, 31);
	pC->Nic.mode.nicFun = NF_RNC_PFB_400_430;
}
void NIC_ReadNetworkStatusPfnet(void)
{
	NIC_ReadRegisters(200, 100);
	pC->Nic.mode.nicFun = NF_RNS_PFN;
}
void NIC_ReadNetworkConfigurationPfnet300_399(void)
{
	NIC_ReadRegisters(300, 100);
	pC->Nic.mode.nicFun = NF_RNC_PFN_300_399;
}
void NIC_ReadNetworkConfigurationPfnet400_499(void)
{
	NIC_ReadRegisters(400, 100);
	pC->Nic.mode.nicFun = NF_RNC_PFN_400_499;
}
void NIC_ReadNetworkConfigurationPfnet500_598(void)
{
	NIC_ReadRegisters(500, 99);
	pC->Nic.mode.nicFun = NF_RNC_PFN_500_598;
}
void NIC_ReadNetworkConfigurationPfnet599_699(void)
{
	NIC_ReadRegisters(599, 101);
	pC->Nic.mode.nicFun = NF_RNC_PFN_599_699;
}
void NIC_ReadNetworkConfigurationPfnet700_799(void)
{
	NIC_ReadRegisters(700, 100);
	pC->Nic.mode.nicFun = NF_RNC_PFN_700_799;
}
void NIC_ReadNetworkConfigurationPfnet800_899(void)
{
	NIC_ReadRegisters(800, 100);
	pC->Nic.mode.nicFun = NF_RNC_PFN_800_899;
}
void NIC_ReadNetworkConfigurationPfnet900_987(void)
{
	NIC_ReadRegisters(900, 88);
	pC->Nic.mode.nicFun = NF_RNC_PFN_900_987;
}

void NIC_WriteStatusMb(void)
{
	uint16_t temptab[STATUS_TABMAX];
	for(uint16_t i=0;i<STATUS_TABMAX;i++)
		temptab[i] = Control_DataSwap(pC->Nic.cod.statusMbtcp[i], pC->Mode.dataSwapMbtcp);
	
	NIC_WriteRegs(2000, 3, temptab, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteStatusPfbus(void)
{
	uint16_t temptab[STATUS_TABMAX];
	for(uint16_t i=0;i<STATUS_TABMAX;i++)
		temptab[i] = Control_DataSwap(pC->Nic.cod.statusPfbus[i], pC->Mode.dataSwapPfbus);
	
	NIC_WriteRegs(2000, 3, temptab, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteStatusPfnet(void)
{
	uint16_t temptab[STATUS_TABMAX];
	for(uint16_t i=0;i<STATUS_TABMAX;i++)
		temptab[i] = Control_DataSwap(pC->Nic.cod.statusPfnet[i], pC->Mode.dataSwapPfnet);
	
	NIC_WriteRegs(2000, 3, temptab, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteSystemConfiguration(void)
{
	NIC_WriteRegs(100, SC_REGMAX, pC->Nic.scWrite.regs, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationMb300_333(void)
{
	NIC_WriteRegs(300, 34, pC->Nic.ncMbWrite.regs, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationPfbus300_399(void)
{
	NIC_WriteRegs(300, 100, pC->Nic.ncPfbusWrite.regs, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationPfbus400_430(void)
{
	NIC_WriteRegs(400, 31, pC->Nic.ncPfbusWrite.regs, 100);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationPfnet300_399(void)
{
	NIC_WriteRegs(300, 100, pC->Nic.ncPfnetWrite.regs, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationPfnet400_499(void)
{
	NIC_WriteRegs(400, 100, pC->Nic.ncPfnetWrite.regs, 100);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationPfnet500_598(void)
{
	NIC_WriteRegs(500, 99, pC->Nic.ncPfnetWrite.regs, 200);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationPfnet599_699(void)
{
	NIC_WriteRegs(599, 101, pC->Nic.ncPfnetWrite.regs, 299);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationPfnet700_799(void)
{
	NIC_WriteRegs(700, 100, pC->Nic.ncPfnetWrite.regs, 400);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationPfnet800_899(void)
{
	NIC_WriteRegs(800, 100, pC->Nic.ncPfnetWrite.regs, 500);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationPfnet900_987(void)
{
	NIC_WriteRegs(900, 88, pC->Nic.ncPfnetWrite.regs, 600);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteClrcfgFlagInCommandFlags(void)
{
	uint16_t val = pC->Nic.cfRead.flagsCommand;
	val |= (1<<6);
	NIC_WriteRegs(1999, 1, &val, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteStrcfgFlagInCommandFlags(void)
{
	uint16_t val = pC->Nic.cfRead.flagsCommand;
	val |= (1<<7);
	NIC_WriteRegs(1999, 1, &val, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteInitFlagInCommandFlags(void)
{
	uint16_t val = pC->Nic.cfRead.flagsCommand;
	val |= (1<<4);
	NIC_WriteRegs(1999, 1, &val, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteCommandFlags(void)
{
	uint16_t val = pC->Nic.cfRead.flagsCommand;
	val |= (1<<4);
	NIC_WriteRegs(1999, 1, &val, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
//**** Funkcje czytajace odpowiedzi z modulu *****************************
static void NIC_ReadResponseAfterReadCoils(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint16_t temp = ((uint16_t)buf[3]<<8) + ((uint16_t)buf[4]<<0);
		if(pC->Mode.protocol == Prot_Mbtcp)
			pC->Nic.cid.coils = Control_DataSwap(temp, pC->Mode.dataSwapMbtcp);
		else if(pC->Mode.protocol == Prot_Pfbus)
			pC->Nic.cid.coils = Control_DataSwap(temp, pC->Mode.dataSwapPfbus);
		else if(pC->Mode.protocol == Prot_Pfnet)
			pC->Nic.cid.coils = Control_DataSwap(temp, pC->Mode.dataSwapPfnet);
	}
}
static void NIC_ReadResponseAfterReadSystemInformations(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
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
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.prodDate);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.ethMACAddr, 0, 6);
		idx += 4; //reserved bytes
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.firm);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.firmVer, 0, 8);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.firmDate, 0, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.firmName, 0, 64);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.comClass);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.protClass);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.si.protConfClass);
		idx += 18; //reserved bytes
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.inputConfShiftRegs, 0, 10);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.si.outputStatusShiftRegs, 0, 10);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.si.regs, 0, 100);
	}
}
static void NIC_ReadResponseAfterReadSystemConfiguration(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
	{
		return;
	}
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
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
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.scRead.reserved, 0, 4);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.offsetAddressOutDataImage);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.scRead.numMappData);
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.scRead.mapData, 0, 78);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.scRead.regs, 0, 100);
	}
}
static void NIC_ReadResponseAfterReadSystemStatusComErrorFlags(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
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
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
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
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.nsMb.regs, 0, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationMb300_333(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
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
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncMbRead.ipAddress, 0, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncMbRead.subnetMask, 0, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncMbRead.gateway, 0, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncMbRead.ethAddress, 0, 6);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncMbRead.flagsReg332_333);
		
		pC->Nic.ncMbRead.responseTimeout *= 100;
		pC->Nic.ncMbRead.clientConWdgTimeout *= 100;
		
		pC->Nic.ncMbRead.flagIpAddressAvailabe 	= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 0) & 0x01);
		pC->Nic.ncMbRead.flagNetMaskAvailabe 		= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 1) & 0x01);
		pC->Nic.ncMbRead.flagGatewayAvailabe 		= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 2) & 0x01);
		pC->Nic.ncMbRead.flagBootIp 						= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 3) & 0x01);
		pC->Nic.ncMbRead.flagDhcp 							= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 4) & 0x01);
		pC->Nic.ncMbRead.flagSetEthAddress 			= (eBool)((pC->Nic.ncMbRead.flagsReg321_322 >> 5) & 0x01);
		
		pC->Nic.ncMbRead.flagMapFc1ToFc3 				= (eBool)((pC->Nic.ncMbRead.flagsReg332_333 >> 0) & 0x01);
		pC->Nic.ncMbRead.flagSkipConfTcpipStack = (eBool)((pC->Nic.ncMbRead.flagsReg332_333 >> 1) & 0x01);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncMbRead.regs, 0, 33);
	}
}
static void NIC_ReadResponseAfterReadNetworkStatusPfbus(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.nsPfbus.regs, 0, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfbus300_399(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
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
		idx++;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfbusRead.confData, 0, 91);
		
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
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfbusRead.regs, 0, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfbus400_430(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfbusRead.confData, 91, 31);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfbusRead.regs, 100, 31);
	}
}
static void NIC_ReadResponseAfterReadNetworkStatusPfnet(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.nsPfnet.regs, 0, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet300_399(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.length);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.busStartup);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.wdgTimeout);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.vendorId);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.deviceId);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.maxAR);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.inputBytes);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.outputBytes);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.lengthNameOfStation);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.nameOfStation, 0, 166);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.regs, 0, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet400_499(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.nameOfStation, 166, 74);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.lengthTypeOfStation);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.typeOfStation, 0, 122);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.regs, 100, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet500_598(void) //99 registers
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.typeOfStation, 122, 118);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.deviceType, 0, 28);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.orderId, 0, 20);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.ipAddress, 0, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.subnetMask, 0, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.gateway, 0, 4);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.hardwareRevision);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.softwareRevision1);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.softwareRevision2);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.softwareRevision3);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.softwareRevisionPrefix);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.maximumDiagRecords);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.instanceId);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.reserved1);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.numApi);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.regs, 200, 99);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet599_699(void) //101 registers
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.profileApi);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.numSubmoduleItem);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.slot);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.subslot);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.moduleId);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.subModuleId);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.provDataLen);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.consDataLen);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.dpmOffsetIn);
		NIC_BytesToUint32(buf, &idx, &pC->Nic.ncPfnetRead.dpmOffsetOut);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.offsetIopsProvider);
		NIC_BytesToUint16(buf, &idx, &pC->Nic.ncPfnetRead.offsetIopsConsumer);
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.reserved2, 0, 4);
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.structureSubmoduleOrApi, 0, 77);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.regs, 299, 101);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet700_799(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.structureSubmoduleOrApi, 77, 100);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.regs, 400, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet800_899(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.structureSubmoduleOrApi, 177, 100);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.regs, 500, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet900_987(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t bytes = buf[2];
	
	if(address != pC->Nic.mode.comAddress)
		return;
	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
		return;
	}
	else
	{
		uint32_t idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.structureSubmoduleOrApi, 277, 88);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.regs, 600, 88);
	}
}
static void NIC_ReadResponseAfterWriteReisters(void)
{
	uint8_t* buf = pC->Nic.bufread;
	if(buf[0] != pC->Nic.mode.comAddress)
		return;
	if(buf[1] != MF_WnHR)
		return;
	uint16_t crc1 = NIC_Crc16(buf, 6);
	uint16_t crc2 = ((uint16_t)buf[6]<<0) + ((uint16_t)buf[7]<<8);
	if(crc1 != crc2)
	{
		pC->Status.flagNicCrcError = true;
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
void NIC_StartComunication(uint8_t num, uint32_t numOfBytes)
{
	pC->Nic.mode.comTimeout = pC->Nic.mode.comOneByteTime * (double)numOfBytes;
	pC->Nic.mode.comTime = 0;
	pC->Nic.mode.maxFunToSend = num;
	pC->Nic.mode.numFunToSend = 0;
	NIC_SendNextFunction();
}
static void NIC_ReadResponse(void)
{
	pC->Status.flagNicCrcError = false;
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
		case NF_RNC_MB_300_333:
			NIC_ReadResponseAfterReadNetworkConfigurationMb300_333();
			break;
		case NF_RNS_PFB:
			NIC_ReadResponseAfterReadNetworkStatusPfbus();
			break;
		case NF_RNC_PFB_300_399:
			NIC_ReadResponseAfterReadNetworkConfigurationPfbus300_399();
			break;
		case NF_RNC_PFB_400_430:
			NIC_ReadResponseAfterReadNetworkConfigurationPfbus400_430();
			break;
		case NF_RNS_PFN:
			NIC_ReadResponseAfterReadNetworkStatusPfnet();
			break;
		case NF_RNC_PFN_300_399:
			NIC_ReadResponseAfterReadNetworkConfigurationPfnet300_399();
			break;
		case NF_RNC_PFN_400_499:
			NIC_ReadResponseAfterReadNetworkConfigurationPfnet400_499();
			break;
		case NF_RNC_PFN_500_598:
			NIC_ReadResponseAfterReadNetworkConfigurationPfnet500_598();
			break;
		case NF_RNC_PFN_599_699:
			NIC_ReadResponseAfterReadNetworkConfigurationPfnet599_699();
			break;
		case NF_RNC_PFN_700_799:
			NIC_ReadResponseAfterReadNetworkConfigurationPfnet700_799();
			break;
		case NF_RNC_PFN_800_899:
			NIC_ReadResponseAfterReadNetworkConfigurationPfnet800_899();
			break;
		case NF_RNC_PFN_900_987:
			NIC_ReadResponseAfterReadNetworkConfigurationPfnet900_987();
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
