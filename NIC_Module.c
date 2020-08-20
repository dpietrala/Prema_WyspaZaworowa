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
static void NIC_ComConf(void)
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
void NIC_Conf(void)
{
	NIC_ComConf();
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
void NIC_BytesToTableUint8(uint8_t* buf, uint32_t* idx, uint8_t* tab, uint32_t num)
{
	for(int i=0;i<num/2;i++)
	{
		tab[2*i+1] = buf[*idx];
		*idx += 1;
		tab[2*i+0] = buf[*idx];
		*idx += 1;
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
static void NIC_WriteRegs(uint16_t addr0, uint16_t numregs, uint16_t* regs, uint32_t startnumreg)
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
		buf[index++] = regs[i + startnumreg] >> 8;
		buf[index++] = regs[i + startnumreg] >> 0;
	}
	uint16_t crc = NIC_Crc16(buf, index);
	buf[index++] = crc >> 0;
	buf[index++] = crc >> 8;
	
	NIC_SendData(buf, index);
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
	pC->Nic.siDefMb.prodCode = 4896;				//doesnt matter
	
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
	pC->Nic.ncMbDef.provSerwerConn = 4;
	pC->Nic.ncMbDef.responseTimeout = 2000;	//2000ms
	pC->Nic.ncMbDef.clientConWdgTimeout = 1000; //1000ms
	pC->Nic.ncMbDef.protMode = 1; //Server mode
	pC->Nic.ncMbDef.sendAckTimeout = 31000; //31000ms
	pC->Nic.ncMbDef.conAckTimeout = 31000; //31000ms
	pC->Nic.ncMbDef.closeAckTimeout = 13000; //13000ms
	pC->Nic.ncMbDef.dataSwap = 1; //Data will be swapped
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
	pC->Nic.siDefPfbus.prodCode = 4907;					//doesnt matter
	
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
	
	pC->Nic.ncPfbusDef.length = 18;	//18 Bayts
	pC->Nic.ncPfbusDef.flagsReg301_302 = 0x00000000;
	pC->Nic.ncPfbusDef.flagBusStartup = false;
	pC->Nic.ncPfbusDef.flagAddressSwitchEnable = false;
	pC->Nic.ncPfbusDef.flagBaudrateSwitchEnable = false;
	pC->Nic.ncPfbusDef.wdgTimeout = 1000;
	pC->Nic.ncPfbusDef.identNumber = 0x000000C10;
	pC->Nic.ncPfbusDef.stationAddress = 2;
	pC->Nic.ncPfbusDef.baudrate = 15;
	
	pC->Nic.ncPfbusDef.flagDpv1Enable = true; //register: 307d bit0
	pC->Nic.ncPfbusDef.flagSyncSupperted = true;
	pC->Nic.ncPfbusDef.flagFreezeSuported = true;
	pC->Nic.ncPfbusDef.flagFailSafeSuported = true;
	pC->Nic.ncPfbusDef.flagAlarmSap50Deactivate = true;
	pC->Nic.ncPfbusDef.flagIoDataSwap = true;
	pC->Nic.ncPfbusDef.flagAutoConfiguration = true;
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
	
	
	pC->Nic.ncPfbusDef.lengthConfData = 2;
	pC->Nic.ncPfbusDef.confData[0] = 0xD0E0;
	
	uint32_t idx = 0;
	NIC_Uint16ToTableUint16(pC->Nic.ncPfbusDef.length, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfbusDef.flagsReg301_302, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfbusDef.wdgTimeout, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfbusDef.identNumber, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint8ToTableUint16(pC->Nic.ncPfbusDef.baudrate, pC->Nic.ncPfbusDef.stationAddress, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfbusDef.flagsReg307, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint8ToTableUint16(pC->Nic.ncPfbusDef.lengthConfData, 0x00, &idx, pC->Nic.ncPfbusDef.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfbusDef.confData[0], &idx, pC->Nic.ncPfbusDef.regs);
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
	pC->Nic.siDefPfnet.prodCode = 4896;				//doesnt matter
	
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
	pC->Nic.ncPfnetDef.length = 844;
	pC->Nic.ncPfnetDef.busStartup = 0;
	pC->Nic.ncPfnetDef.wdgTimeout = 1000;
	pC->Nic.ncPfnetDef.vendorId = 0x011E;
	pC->Nic.ncPfnetDef.deviceId = 0x00000010A;
	pC->Nic.ncPfnetDef.maxAR = 0;
	pC->Nic.ncPfnetDef.inputBytes = 128;
	pC->Nic.ncPfnetDef.outputBytes = 128;
	
	pC->Nic.ncPfnetDef.lengthNameOfStation = 0;
	pC->Nic.ncPfnetDef.nameOfStation[0] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[1] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[2] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[3] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[4] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[5] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[6] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[7] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[8] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[9] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[10] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[11] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[12] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[13] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[14] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[15] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[16] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[17] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[18] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[19] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[20] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[21] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[22] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[23] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[24] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[25] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[26] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[27] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[28] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[29] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[30] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[31] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[32] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[33] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[34] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[35] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[36] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[37] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[38] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[39] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[40] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[41] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[42] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[43] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[44] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[45] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[46] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[47] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[48] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[49] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[50] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[51] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[52] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[53] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[54] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[55] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[56] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[57] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[58] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[59] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[60] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[61] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[62] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[63] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[64] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[65] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[66] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[67] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[68] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[69] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[70] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[71] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[72] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[73] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[74] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[75] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[76] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[77] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[78] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[79] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[80] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[81] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[82] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[83] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[84] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[85] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[86] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[87] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[88] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[89] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[90] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[91] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[92] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[93] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[94] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[95] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[96] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[97] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[98] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[99] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[100] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[101] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[102] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[103] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[104] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[105] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[106] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[107] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[108] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[109] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[110] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[111] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[112] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[113] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[114] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[115] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[116] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[117] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[118] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[119] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[120] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[121] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[122] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[123] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[124] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[125] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[126] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[127] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[128] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[129] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[130] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[131] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[132] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[133] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[134] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[135] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[136] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[137] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[138] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[139] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[140] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[141] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[142] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[143] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[144] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[145] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[146] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[147] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[148] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[149] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[150] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[151] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[152] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[153] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[154] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[155] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[156] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[157] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[158] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[159] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[160] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[161] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[162] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[163] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[164] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[165] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[166] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[167] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[168] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[169] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[170] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[171] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[172] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[173] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[174] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[175] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[176] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[177] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[178] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[179] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[180] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[181] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[182] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[183] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[184] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[185] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[186] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[187] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[188] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[189] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[190] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[191] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[192] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[193] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[194] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[195] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[196] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[197] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[198] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[199] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[200] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[201] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[202] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[203] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[204] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[205] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[206] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[207] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[208] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[209] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[210] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[211] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[212] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[213] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[214] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[215] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[216] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[217] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[218] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[219] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[220] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[221] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[222] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[223] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[224] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[225] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[226] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[227] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[228] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[229] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[230] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[231] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[232] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[233] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[234] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[235] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[236] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[237] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[238] = 0;
	pC->Nic.ncPfnetDef.nameOfStation[239] = 0;
	
	pC->Nic.ncPfnetDef.lengthTypeOfStation = 0;
	pC->Nic.ncPfnetDef.typeOfStation[0] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[1] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[2] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[3] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[4] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[5] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[6] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[7] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[8] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[9] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[10] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[11] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[12] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[13] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[14] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[15] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[16] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[17] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[18] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[19] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[20] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[21] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[22] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[23] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[24] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[25] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[26] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[27] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[28] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[29] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[30] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[31] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[32] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[33] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[34] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[35] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[36] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[37] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[38] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[39] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[40] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[41] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[42] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[43] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[44] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[45] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[46] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[47] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[48] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[49] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[50] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[51] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[52] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[53] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[54] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[55] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[56] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[57] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[58] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[59] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[60] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[61] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[62] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[63] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[64] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[65] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[66] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[67] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[68] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[69] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[70] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[71] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[72] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[73] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[74] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[75] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[76] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[77] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[78] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[79] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[80] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[81] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[82] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[83] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[84] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[85] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[86] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[87] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[88] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[89] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[90] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[91] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[92] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[93] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[94] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[95] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[96] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[97] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[98] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[99] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[100] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[101] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[102] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[103] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[104] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[105] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[106] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[107] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[108] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[109] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[110] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[111] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[112] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[113] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[114] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[115] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[116] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[117] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[118] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[119] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[120] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[121] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[122] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[123] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[124] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[125] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[126] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[127] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[128] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[129] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[130] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[131] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[132] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[133] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[134] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[135] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[136] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[137] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[138] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[139] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[140] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[141] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[142] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[143] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[144] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[145] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[146] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[147] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[148] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[149] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[150] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[151] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[152] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[153] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[154] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[155] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[156] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[157] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[158] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[159] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[160] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[161] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[162] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[163] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[164] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[165] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[166] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[167] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[168] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[169] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[170] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[171] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[172] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[173] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[174] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[175] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[176] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[177] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[178] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[179] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[180] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[181] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[182] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[183] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[184] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[185] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[186] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[187] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[188] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[189] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[190] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[191] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[192] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[193] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[194] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[195] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[196] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[197] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[198] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[199] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[200] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[201] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[202] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[203] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[204] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[205] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[206] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[207] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[208] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[209] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[210] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[211] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[212] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[213] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[214] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[215] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[216] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[217] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[218] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[219] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[220] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[221] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[222] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[223] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[224] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[225] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[226] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[227] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[228] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[229] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[230] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[231] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[232] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[233] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[234] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[235] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[236] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[237] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[238] = 0;
	pC->Nic.ncPfnetDef.typeOfStation[239] = 0;
	

	pC->Nic.ncPfnetDef.deviceType[0] = 0;
	pC->Nic.ncPfnetDef.deviceType[1] = 0;
	pC->Nic.ncPfnetDef.deviceType[2] = 0;
	pC->Nic.ncPfnetDef.deviceType[3] = 0;
	pC->Nic.ncPfnetDef.deviceType[4] = 0;
	pC->Nic.ncPfnetDef.deviceType[5] = 0;
	pC->Nic.ncPfnetDef.deviceType[6] = 0;
	pC->Nic.ncPfnetDef.deviceType[7] = 0;
	pC->Nic.ncPfnetDef.deviceType[8] = 0;
	pC->Nic.ncPfnetDef.deviceType[9] = 0;
	pC->Nic.ncPfnetDef.deviceType[10] = 0;
	pC->Nic.ncPfnetDef.deviceType[11] = 0;
	pC->Nic.ncPfnetDef.deviceType[12] = 0;
	pC->Nic.ncPfnetDef.deviceType[13] = 0;
	pC->Nic.ncPfnetDef.deviceType[14] = 0;
	pC->Nic.ncPfnetDef.deviceType[15] = 0;
	pC->Nic.ncPfnetDef.deviceType[16] = 0;
	pC->Nic.ncPfnetDef.deviceType[17] = 0;
	pC->Nic.ncPfnetDef.deviceType[18] = 0;
	pC->Nic.ncPfnetDef.deviceType[19] = 0;
	pC->Nic.ncPfnetDef.deviceType[20] = 0;
	pC->Nic.ncPfnetDef.deviceType[21] = 0;
	pC->Nic.ncPfnetDef.deviceType[22] = 0;
	pC->Nic.ncPfnetDef.deviceType[23] = 0;
	pC->Nic.ncPfnetDef.deviceType[24] = 0;
	pC->Nic.ncPfnetDef.deviceType[25] = 0;
	pC->Nic.ncPfnetDef.deviceType[26] = 0;
	pC->Nic.ncPfnetDef.deviceType[27] = 0;
	
	pC->Nic.ncPfnetDef.orderId[0] = 0;
	pC->Nic.ncPfnetDef.orderId[1] = 0;
	pC->Nic.ncPfnetDef.orderId[2] = 0;
	pC->Nic.ncPfnetDef.orderId[3] = 0;
	pC->Nic.ncPfnetDef.orderId[4] = 0;
	pC->Nic.ncPfnetDef.orderId[5] = 0;
	pC->Nic.ncPfnetDef.orderId[6] = 0;
	pC->Nic.ncPfnetDef.orderId[7] = 0;
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
	
	pC->Nic.ncPfnetDef.ipAddress[0] = 0;
	pC->Nic.ncPfnetDef.ipAddress[1] = 0;
	pC->Nic.ncPfnetDef.ipAddress[2] = 0;
	pC->Nic.ncPfnetDef.ipAddress[3] = 0;
	pC->Nic.ncPfnetDef.subnetMask[0] = 0;
	pC->Nic.ncPfnetDef.subnetMask[1] = 0;
	pC->Nic.ncPfnetDef.subnetMask[2] = 0;
	pC->Nic.ncPfnetDef.subnetMask[3] = 0;
	pC->Nic.ncPfnetDef.gateway[0] = 0;
	pC->Nic.ncPfnetDef.gateway[1] = 0;
	pC->Nic.ncPfnetDef.gateway[2] = 0;
	pC->Nic.ncPfnetDef.gateway[3] = 0;
	
	pC->Nic.ncPfnetDef.hardwareRevision = 0;
	pC->Nic.ncPfnetDef.softwareRevision1 = 0;
	pC->Nic.ncPfnetDef.softwareRevision2 = 0;
	pC->Nic.ncPfnetDef.softwareRevision3 = 0;
	pC->Nic.ncPfnetDef.softwareRevisionPrefix = ((uint16_t)'R' << 8);
	
	pC->Nic.ncPfnetDef.maximumDiagRecords = 256;
	pC->Nic.ncPfnetDef.instanceId = 1;
	pC->Nic.ncPfnetDef.reserved1 = 0;
	
	pC->Nic.ncPfnetDef.numApi = 1;
	pC->Nic.ncPfnetDef.profileApi = 0;
	pC->Nic.ncPfnetDef.numSubmoduleItem = 1;
	pC->Nic.ncPfnetDef.slot = 0;
	pC->Nic.ncPfnetDef.subslot = 0;
	pC->Nic.ncPfnetDef.subModuleId = 0;
	pC->Nic.ncPfnetDef.provDataLen = 0;
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
	
	for(uint32_t i=0;i<365;i++)
		pC->Nic.ncPfnetDef.structureSubmoduleOrApi[i] = 0;


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
void NIC_ReadNetworkConfigurationMb300_332(void)
{
	NIC_ReadRegisters(300, 33);
	pC->Nic.mode.nicFun = NF_RNC_MB_300_332;
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
void NIC_ReadNetworkConfigurationPfnet500_599(void)
{
	NIC_ReadRegisters(500, 100);
	pC->Nic.mode.nicFun = NF_RNC_PFN_500_599;
}
void NIC_ReadNetworkConfigurationPfnet600_699(void)
{
	NIC_ReadRegisters(600, 100);
	pC->Nic.mode.nicFun = NF_RNC_PFN_600_699;
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

void NIC_WriteCoils(void)
{
	NIC_WriteRegs(2000, 1, &pC->Nic.cod.coils, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteSystemConfiguration(void)
{
	pC->Nic.scWrite = pC->Nic.scRead;
	pC->Nic.scWrite.regs[4] = 5;
	NIC_WriteRegs(100, 100, pC->Nic.scWrite.regs, 0);
	pC->Nic.mode.nicFun = NF_WR;
}
void NIC_WriteNetworkConfigurationMb300_332(void)
{
	NIC_WriteRegs(300, 33, pC->Nic.ncMbWrite.regs, 0);
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
void NIC_WriteNetworkConfigurationPfnet(void)
{
//	NIC_WriteRegs(300, 100, pC->Nic.ncPfnetWrite.regs);
//	pC->Nic.mode.nicFun = NF_WR;
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
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.si.regs, 0, 100);
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
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.nsMb.regs, 0, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationMb300_332(void)
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
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.nsPfbus.regs, 0, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfbus300_399(void)
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
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.nsPfnet.regs, 0, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet300_399(void)
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
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.ipAddress, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.subnetMask, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.gateway, 4);
		
		idx = 3;
		NIC_BytesToTableUint16(buf, &idx, pC->Nic.ncPfnetRead.regs, 0, 100);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet400_499(void)
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
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.ipAddress, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.subnetMask, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.gateway, 4);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet500_599(void)
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
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.ipAddress, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.subnetMask, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.gateway, 4);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet600_699(void)
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
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.ipAddress, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.subnetMask, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.gateway, 4);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet700_799(void)
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
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.ipAddress, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.subnetMask, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.gateway, 4);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet800_899(void)
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
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.ipAddress, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.subnetMask, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.gateway, 4);
	}
}
static void NIC_ReadResponseAfterReadNetworkConfigurationPfnet900_987(void)
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
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.ipAddress, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.subnetMask, 4);
		NIC_BytesToTableUint8(buf, &idx, pC->Nic.ncPfnetRead.gateway, 4);
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
void NIC_StartComunication(uint8_t num, uint32_t timeout)
{
	pC->Nic.mode.comTimeout = timeout;
	pC->Nic.mode.comTime = 0;
	pC->Nic.mode.maxFunToSend = num;
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
		case NF_RNC_MB_300_332:
			NIC_ReadResponseAfterReadNetworkConfigurationMb300_332();
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
		case NF_RNC_PFN_500_599:
			NIC_ReadResponseAfterReadNetworkConfigurationPfnet500_599();
			break;
		case NF_RNC_PFN_600_699:
			NIC_ReadResponseAfterReadNetworkConfigurationPfnet600_699();
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
