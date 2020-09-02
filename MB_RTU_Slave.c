#include "MB_RTU_Slave.h"
extern sControl* pC;
static void MBS_SetSend(void)
{
	for(uint32_t i=0;i<100000;i++)
		;
	GPIOA->ODR |= GPIO_ODR_OD11;
}
static void MBS_SetRead(void)
{
	GPIOA->ODR &= ~GPIO_ODR_OD11;
}
eResult MBS_ComConf(void)
{
	eResult result = RES_OK;
	
	DMA2_Stream2->CR = 0x0000;
	DMA2_Stream7->CR = 0x0000;
	DMA2->LIFCR |= DMA_LIFCR_CTCIF2;
	DMA2->HIFCR |= DMA_HIFCR_CTCIF7;
	
	MBS_ClrStr(pC->Mbs.bufread, MBS_BUFMAX);
	MBS_ClrStr(pC->Mbs.bufwrite, MBS_BUFMAX);
	
	DMA2_Stream2->PAR 	= (uint32_t)&USART1->DR;
	DMA2_Stream2->M0AR 	= (uint32_t)pC->Mbs.bufread;
	DMA2_Stream2->NDTR 	= (uint16_t)MBS_BUFMAX;
	DMA2_Stream2->CR 		= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_EN;
	DMA2_Stream7->PAR 	= (uint32_t)&USART1->DR;
	DMA2_Stream7->M0AR 	= (uint32_t)pC->Mbs.bufwrite;
	DMA2_Stream7->NDTR 	= (uint16_t)MBS_BUFMAX;
	DMA2_Stream7->CR 		= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_DIR_0 | DMA_SxCR_TCIE;
	NVIC_EnableIRQ(DMA2_Stream7_IRQn);
	
	GPIOA->MODER 	|= GPIO_MODER_MODER11_0;
	GPIOA->PUPDR 	|= GPIO_PUPDR_PUPD11_1;
	MBS_SetRead();
	
	GPIOA->MODER 	|= GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1;
	GPIOA->PUPDR 	|= GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR10_0;
	GPIOA->AFR[1] = 0x00000770;
	USART1->BRR 	= 50000000/pC->Mbs.baud;
	USART1->CR3 	= USART_CR3_DMAR | USART_CR3_DMAT;
	
	if(pC->Mbs.parity == 0)
		USART1->CR1 	= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE;
	else if(pC->Mbs.parity == 1)
		USART1->CR1 	= USART_CR1_PCE | USART_CR1_PS | USART_CR1_M | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE;
	else if(pC->Mbs.parity == 2)
		USART1->CR1 	= USART_CR1_PCE | USART_CR1_M | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE;

	NVIC_EnableIRQ(USART1_IRQn);
	
	return result;
}
static void MBS_StartSending(uint16_t num)
{
	MBS_SetSend();
	DMA2_Stream7->CR &= ~DMA_SxCR_EN;
	DMA2->HIFCR |= DMA_HIFCR_CTCIF7;
	DMA2_Stream7->NDTR = num;
	DMA2_Stream7->CR |= DMA_SxCR_EN;
}
static void MBS_ReloadDmaToRead(void)
{
	DMA2_Stream2->CR &= ~DMA_SxCR_EN;
	DMA2->LIFCR |= DMA_LIFCR_CTCIF2;
	DMA2_Stream2->NDTR = MBS_BUFMAX;
	DMA2_Stream2->CR |= DMA_SxCR_EN;
}
void MBS_ClrStr(uint8_t* str, uint32_t n)
{
	for(uint32_t i=0;i<n;i++)
		str[i] = 0x00;
}
uint16_t MBS_Crc16(uint8_t* buf, uint32_t len)
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
// ----- ModbusRTU responses -----------------------------------------
static void MBS_ResponseError_IF(void)
{
	pC->Status.flagBusMbrtuErrorIllegalFunction = true;
	pC->Status.statBusMbrtuCounterError++;
	pC->Mbs.error = MFE_IF;
	uint8_t* bufw = pC->Mbs.bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	uint16_t idx = 0;
	bufw[idx++] = pC->Mbs.address;
	bufw[idx++] = 0x80 + pC->Mbs.fun;
	bufw[idx++] = pC->Mbs.error;
	uint16_t crc = MBS_Crc16(bufw, idx);
	bufw[idx++] = crc;
	bufw[idx++] = crc >> 8;
	MBS_StartSending(idx);
}
static void MBS_ResponseError_IDR(void)
{
	pC->Status.flagBusMbrtuErrorIllegalDataRange = true;
	pC->Status.statBusMbrtuCounterError++;
	pC->Mbs.error = MFE_IDR;
	uint8_t* bufw = pC->Mbs.bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	uint16_t idx = 0;
	bufw[idx++] = pC->Mbs.address;
	bufw[idx++] = 0x80 + pC->Mbs.fun;
	bufw[idx++] = pC->Mbs.error;
	uint16_t crc = MBS_Crc16(bufw, idx);
	bufw[idx++] = crc;
	bufw[idx++] = crc >> 8;
	MBS_StartSending(idx);
}
static void MBS_Response_RnIR(void)
{
	for(uint16_t i=0;i<MBS_IREGMAX;i++)
		pC->Mbs.iregs[i] = pC->Mbs.statusMbrtu[i];
	uint8_t* bufr = pC->Mbs.bufread;
	uint8_t* bufw = pC->Mbs.bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	uint16_t regstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
	uint16_t regnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
	uint16_t regend = regstart + regnum - 1;
	uint16_t numbytes = 2 * regnum;
	
	uint16_t index = 0;
	bufw[index++] = pC->Mbs.address;
	bufw[index++] = pC->Mbs.fun;
	bufw[index++] = numbytes;
	for(int i=regstart;i<=regend;i++)
	{
		uint16_t temp = Control_DataSwap(pC->Mbs.iregs[i], pC->Mode.dataSwapMbrtu);
		bufw[index++] = temp >> 8;
		bufw[index++] = temp >> 0;
	}
	uint16_t crc = MBS_Crc16(bufw, index);
	bufw[index++] = crc;
	bufw[index++] = crc >> 8;
	
	MBS_StartSending(index);
}
static void MBS_Response_W1HR(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	uint8_t* bufw = pC->Mbs.bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	
	uint16_t temp = ((uint16_t)bufr[4]<<8);
	temp += ((uint16_t)bufr[5]<<0);
	pC->Mbs.hregs[0] = Control_DataSwap(temp, pC->Mode.dataSwapMbrtu);
	
	for(int i=0;i<6;i++)
		bufw[i] = bufr[i];
	uint16_t crc = MBS_Crc16(bufw, 6);
	bufw[6] = crc;
	bufw[7] = crc >> 8;

	MBS_StartSending(8);
}
static void MBS_Response_WnHR(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	uint8_t* bufw = pC->Mbs.bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	uint16_t regstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
	uint16_t regnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
	uint16_t regend = regstart + regnum - 1;
	uint16_t index = 7;
	for(int i=regstart;i<=regend;i++)
	{
		uint16_t temp = ((uint16_t)bufr[index++]<<8);
		temp += ((uint16_t)bufr[index++]<<0);
		pC->Mbs.hregs[i] = Control_DataSwap(temp, pC->Mode.dataSwapMbrtu);
	}
	
	for(int i=0;i<6;i++)
		bufw[i] = bufr[i];
	uint16_t crc = MBS_Crc16(bufw, 6);
	bufw[6] = crc;
	bufw[7] = crc >> 8;

	MBS_StartSending(8);
}
// ----- ModbusRTU requests -----------------------------------------
static void MBS_ReadRequest_RnDQ(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	pC->Mbs.fun = (eMBFun)bufr[1];
	MBS_ResponseError_IF();
}
static void MBS_ReadRequest_RnDI(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	pC->Mbs.fun = (eMBFun)bufr[1];
	MBS_ResponseError_IF();
}
static void MBS_ReadRequest_RnHR(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	pC->Mbs.fun = (eMBFun)bufr[1];
	MBS_ResponseError_IF();
}
static void MBS_ReadRequest_RnIR(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	uint16_t crc1 = MBS_Crc16(bufr, 6);
	uint16_t crc2 = ((uint16_t)bufr[6]<<0) + ((uint16_t)bufr[7]<<8);
	if(crc1 == crc2)
	{
		pC->Mbs.time = 0;
		pC->Status.flagBusMbrtuErrorIllegalFunction = false;
		pC->Mbs.fun = MF_RnIR;
		uint16_t regstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
		uint16_t regnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
		uint16_t regend = regstart + regnum - 1;
		if(regend >= MBS_IREGMAX)
		{
			MBS_ResponseError_IDR();
		}
		else
		{
			pC->Status.flagBusMbrtuErrorIllegalDataRange = false;
			MBS_Response_RnIR();
		}
	}
}
static void MBS_ReadRequest_W1DQ(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	pC->Mbs.fun = (eMBFun)bufr[1];
	MBS_ResponseError_IF();
}
static void MBS_ReadRequest_W1HR(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	uint16_t crc1 = MBS_Crc16(bufr, 6);
	uint16_t crc2 = ((uint16_t)bufr[6]<<0) + ((uint16_t)bufr[7]<<8);
	if(crc1 == crc2)
	{
		pC->Mbs.time = 0;
		pC->Status.flagBusMbrtuErrorIllegalFunction = false;
		pC->Mbs.fun = MF_W1HR;
		uint16_t regstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
		if(regstart >= MBS_HREGMAX)
		{
			MBS_ResponseError_IDR();
		}
		else
		{
			pC->Status.flagBusMbrtuErrorIllegalDataRange = false;
			MBS_Response_W1HR();
		}
	}
}
static void MBS_ReadRequest_RS(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	pC->Mbs.fun = (eMBFun)bufr[1];
	MBS_ResponseError_IF();
}
static void MBS_ReadRequest_DT(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	pC->Mbs.fun = (eMBFun)bufr[1];
	MBS_ResponseError_IF();
}
static void MBS_ReadRequest_WnDQ(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	pC->Mbs.fun = (eMBFun)bufr[1];
	MBS_ResponseError_IF();
}
static void MBS_ReadRequest_WnHR(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	uint8_t numbytes = 7 + bufr[6];
	uint16_t crc1 = MBS_Crc16(bufr, numbytes);
	uint16_t crc2 = ((uint16_t)bufr[numbytes]<<0) + ((uint16_t)bufr[numbytes+1]<<8);
	if(crc1 == crc2)
	{
		pC->Mbs.time = 0;
		pC->Status.flagBusMbrtuErrorIllegalFunction = false;
		pC->Mbs.fun = MF_WnHR;
		uint16_t regstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
		uint16_t regnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
		uint16_t regend = regstart + regnum - 1;
		if(regend >= MBS_HREGMAX)
		{
			MBS_ResponseError_IDR();
		}
		else
		{
			pC->Status.flagBusMbrtuErrorIllegalDataRange = false;
			MBS_Response_WnHR();
		}
	}
}
static void MBS_ReadRequest_IF(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	pC->Mbs.fun = (eMBFun)bufr[1];
	MBS_ResponseError_IF();
}
// ----- ModbusRTU general -----------------------------------------
static void MBS_ReadRequest(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	switch(buf[1])
	{
		case MF_RnDQ:		MBS_ReadRequest_RnDQ();	break;
		case MF_RnDI:		MBS_ReadRequest_RnDI();	break;
		case MF_RnHR:		MBS_ReadRequest_RnHR();	break;
		case MF_RnIR:		MBS_ReadRequest_RnIR();	break;
		case MF_W1DQ:		MBS_ReadRequest_W1DQ();	break;
		case MF_W1HR:		MBS_ReadRequest_W1HR();	break;
		case MF_RS:			MBS_ReadRequest_RS();		break;
		case MF_DT:			MBS_ReadRequest_DT();		break;
		case MF_WnDQ:		MBS_ReadRequest_WnDQ();	break;
		case MF_WnHR:		MBS_ReadRequest_WnHR();	break;
		default:				MBS_ReadRequest_IF();		break;
	}
}
// Funkcje zwiazane z konfiguracja z poziomu PC **************************************
static uint16_t Config_Crc16(uint8_t* packet, uint32_t nBytes)
{
	uint16_t crc = 0;
	for(uint32_t byte = 0; byte < nBytes; byte++)
	{
		crc = crc ^ ((uint16_t)packet[byte] << 8);
		for (uint8_t bit = 0; bit < 8; bit++)
			if(crc & 0x8000) 
				crc = (crc << 1) ^ 0x1021;
			else
				crc = crc << 1;
	}
	return crc;
}
static void Config_SendConfStmToPc(void)
{
	uint8_t* buf = pC->Mbs.bufwrite;
	MBS_ClrStr(buf, MBS_BUFMAX);
	uint32_t idx = 0;
	buf[idx++] = 249;
	buf[idx++] = 250;
	buf[idx++] = 251;
	buf[idx++] = 252;
	buf[idx++] = 253;
	buf[idx++] = (uint8_t)frameConfig_ConfStmToPc;
	
	buf[idx++] = pC->Ee.rData[EeAdd_stmProt] >> 0;
	
	buf[idx++] = pC->Ee.rData[EeAdd_mbrtuTimeout] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_mbrtuTimeout] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbrtuDataSwap] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbrtuAddress] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbrtuBaudrate] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbrtuParity] >> 0;
	
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpTimeout] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpTimeout] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpDataSwap] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpIP0] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpIP1] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpIP2] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpIP3] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpMask0] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpMask1] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpMask2] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpMask3] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpGateway0] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpGateway1] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpGateway2] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpGateway3] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpSerwerCons] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpSendAckTimeoutHigh] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpSendAckTimeoutHigh] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpSendAckTimeoutLow] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpSendAckTimeoutLow] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpConnectAckTimeoutHigh] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpConnectAckTimeoutHigh] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpConnectAckTimeoutLow] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpConnectAckTimeoutLow] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpCloseAckTimeoutHigh] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpCloseAckTimeoutHigh] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpCloseAckTimeoutLow] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_mbtcpCloseAckTimeoutLow] >> 0;

	buf[idx++] = pC->Ee.rData[EeAdd_pfbusTimeout] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_pfbusTimeout] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfbusDataSwap] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfbusIdentNumber] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_pfbusIdentNumber] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfbusAdress] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfbusBaudrate] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfbusDPV1Enable] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfbusSyncSupported] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfbusFreezeSupported] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfbusFailSafeSupported] >> 0;
	
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetTimeout] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetTimeout] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetDataSwap] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetVendorId] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetVendorId] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetDeviceId] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetDeviceId] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetLengthNameOfStation] >> 0;
	for(uint8_t i=0;i<240;i++)
		buf[idx++] = pC->Ee.rData[EeAdd_pfnetNameOfStation + i] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetLengthTypeOfStation] >> 0;
	for(uint8_t i=0;i<240;i++)
		buf[idx++] = pC->Ee.rData[EeAdd_pfnetTypeOfStation + i] >> 0;
	for(uint8_t i=0;i<28;i++)
		buf[idx++] = pC->Ee.rData[EeAdd_pfnetDeviceType + i] >> 0;
	for(uint8_t i=0;i<20;i++)
		buf[idx++] = pC->Ee.rData[EeAdd_pfnetOrderId + i] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetIP0] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetIP1] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetIP2] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetIP3] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetMask0] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetMask1] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetMask2] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetMask3] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetGateway0] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetGateway1] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetGateway2] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetGateway3] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetHardwareRevision] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetHardwareRevision] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetSoftwareRevision1] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetSoftwareRevision1] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetSoftwareRevision2] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetSoftwareRevision2] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetSoftwareRevision3] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetSoftwareRevision3] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetSoftwareRevisionPrefix] >> 0;
	
	uint16_t crc = Config_Crc16(buf, idx);
	buf[idx++] = crc >> 8;
	buf[idx++] = crc >> 0;
	
	MBS_StartSending(idx);
}
static void Config_SendTelemStmToPc(void)
{
	uint8_t* buf = pC->Mbs.bufwrite;
	MBS_ClrStr(buf, MBS_BUFMAX);
	uint32_t idx = 0;
	buf[idx++] = 249;
	buf[idx++] = 250;
	buf[idx++] = 251;
	buf[idx++] = 252;
	buf[idx++] = 253;
	buf[idx++] = (uint8_t)frameConfig_TelemStmToPc;
	
	buf[idx++] = pC->Outs.coils >> 8;
	buf[idx++] = pC->Outs.coils >> 0;
	buf[idx++] = pC->Mode.workType >> 0;
	buf[idx++] = pC->Mode.protocol >> 0;
	buf[idx++] = (uint16_t)((int16_t)(pC->Status.statTemp * 10)) >> 8;
	buf[idx++] = (uint16_t)((int16_t)(pC->Status.statTemp * 10)) >> 0;
	buf[idx++] = pC->Status.flagsStm32 >> 8;
	buf[idx++] = pC->Status.flagsStm32 >> 0;
	
	buf[idx++] = pC->Nic.si.devNumber >> 24;
	buf[idx++] = pC->Nic.si.devNumber >> 16;
	buf[idx++] = pC->Nic.si.devNumber >> 8;
	buf[idx++] = pC->Nic.si.devNumber >> 0;
	buf[idx++] = pC->Nic.si.serNumber >> 24;
	buf[idx++] = pC->Nic.si.serNumber >> 16;
	buf[idx++] = pC->Nic.si.serNumber >> 8;
	buf[idx++] = pC->Nic.si.serNumber >> 0;
	buf[idx++] = pC->Nic.si.devClass >> 8;
	buf[idx++] = pC->Nic.si.devClass >> 0;
	buf[idx++] = pC->Nic.si.hardRev >> 0;
	buf[idx++] = pC->Nic.si.hardCompIndex >> 0;
	buf[idx++] = pC->Nic.si.hardOpChann0 >> 8;
	buf[idx++] = pC->Nic.si.hardOpChann0 >> 0;
	buf[idx++] = pC->Nic.si.hardOpChann1 >> 8;
	buf[idx++] = pC->Nic.si.hardOpChann1 >> 0;
	buf[idx++] = pC->Nic.si.hardOpChann2 >> 8;
	buf[idx++] = pC->Nic.si.hardOpChann2 >> 0;
	buf[idx++] = pC->Nic.si.hardOpChann3 >> 8;
	buf[idx++] = pC->Nic.si.hardOpChann3 >> 0;
	buf[idx++] = pC->Nic.si.virtualDPMSize >> 24;
	buf[idx++] = pC->Nic.si.virtualDPMSize >> 16;
	buf[idx++] = pC->Nic.si.virtualDPMSize >> 8;
	buf[idx++] = pC->Nic.si.virtualDPMSize >> 0;
	buf[idx++] = pC->Nic.si.manufCode >> 8;
	buf[idx++] = pC->Nic.si.manufCode >> 0;
	buf[idx++] = pC->Nic.si.prodDate >> 8;
	buf[idx++] = pC->Nic.si.prodDate >> 0;
	for(uint8_t i=0;i<6;i++)
		buf[idx++] = pC->Nic.si.ethMACAddr[i] >> 0;
	for(uint8_t i=0;i<8;i++)
		buf[idx++] = pC->Nic.si.firmVer[i] >> 0;
	for(uint8_t i=0;i<4;i++)
		buf[idx++] = pC->Nic.si.firmDate[i] >> 0;
	for(uint8_t i=0;i<64;i++)
		buf[idx++] = pC->Nic.si.firmName[i] >> 0;
	buf[idx++] = pC->Nic.si.comClass >> 8;
	buf[idx++] = pC->Nic.si.comClass >> 0;
	buf[idx++] = pC->Nic.si.protClass >> 8;
	buf[idx++] = pC->Nic.si.protClass >> 0;
	buf[idx++] = pC->Nic.si.protConfClass >> 8;
	buf[idx++] = pC->Nic.si.protConfClass >> 0;
	
	buf[idx++] = pC->Status.flagsMbrtu >> 8;
	buf[idx++] = pC->Status.flagsMbrtu >> 0;
	buf[idx++] = pC->Status.statBusMbrtuCounterError >> 8;
	buf[idx++] = pC->Status.statBusMbrtuCounterError >> 0;
	
	buf[idx++] = pC->Status.flagsMbtcp >> 8;
	buf[idx++] = pC->Status.flagsMbtcp >> 0;
	buf[idx++] = pC->Status.statBusMbtcpCounterError >> 8;
	buf[idx++] = pC->Status.statBusMbtcpCounterError >> 0;
	buf[idx++] = pC->Status.statBusMbtcpSystemError >> 24;
	buf[idx++] = pC->Status.statBusMbtcpSystemError >> 16;
	buf[idx++] = pC->Status.statBusMbtcpSystemError >> 8;
	buf[idx++] = pC->Status.statBusMbtcpSystemError >> 0;
	buf[idx++] = pC->Status.statBusMbtcpComError >> 24;
	buf[idx++] = pC->Status.statBusMbtcpComError >> 16;
	buf[idx++] = pC->Status.statBusMbtcpComError >> 8;
	buf[idx++] = pC->Status.statBusMbtcpComError >> 0;
	buf[idx++] = pC->Status.statBusMbtcpComStatus >> 24;
	buf[idx++] = pC->Status.statBusMbtcpComStatus >> 16;
	buf[idx++] = pC->Status.statBusMbtcpComStatus >> 8;
	buf[idx++] = pC->Status.statBusMbtcpComStatus >> 0;
	
	buf[idx++] = pC->Status.flagsPfbus >> 8;
	buf[idx++] = pC->Status.flagsPfbus >> 0;
	buf[idx++] = pC->Status.statBusPfbusCounterError >> 8;
	buf[idx++] = pC->Status.statBusPfbusCounterError >> 0;
	buf[idx++] = pC->Status.statBusPfbusSystemError >> 24;
	buf[idx++] = pC->Status.statBusPfbusSystemError >> 16;
	buf[idx++] = pC->Status.statBusPfbusSystemError >> 8;
	buf[idx++] = pC->Status.statBusPfbusSystemError >> 0;
	buf[idx++] = pC->Status.statBusPfbusComError >> 24;
	buf[idx++] = pC->Status.statBusPfbusComError >> 16;
	buf[idx++] = pC->Status.statBusPfbusComError >> 8;
	buf[idx++] = pC->Status.statBusPfbusComError >> 0;
	buf[idx++] = pC->Status.statBusPfbusComStatus >> 24;
	buf[idx++] = pC->Status.statBusPfbusComStatus >> 16;
	buf[idx++] = pC->Status.statBusPfbusComStatus >> 8;
	buf[idx++] = pC->Status.statBusPfbusComStatus >> 0;
	
	buf[idx++] = pC->Status.flagsPfnet >> 8;
	buf[idx++] = pC->Status.flagsPfnet >> 0;
	buf[idx++] = pC->Status.statBusPfnetCounterError >> 8;
	buf[idx++] = pC->Status.statBusPfnetCounterError >> 0;
	buf[idx++] = pC->Status.statBusPfnetSystemError >> 24;
	buf[idx++] = pC->Status.statBusPfnetSystemError >> 16;
	buf[idx++] = pC->Status.statBusPfnetSystemError >> 8;
	buf[idx++] = pC->Status.statBusPfnetSystemError >> 0;
	buf[idx++] = pC->Status.statBusPfnetComError >> 24;
	buf[idx++] = pC->Status.statBusPfnetComError >> 16;
	buf[idx++] = pC->Status.statBusPfnetComError >> 8;
	buf[idx++] = pC->Status.statBusPfnetComError >> 0;
	buf[idx++] = pC->Status.statBusPfnetComStatus >> 24;
	buf[idx++] = pC->Status.statBusPfnetComStatus >> 16;
	buf[idx++] = pC->Status.statBusPfnetComStatus >> 8;
	buf[idx++] = pC->Status.statBusPfnetComStatus >> 0;
	
	uint16_t crc = Config_Crc16(buf, idx);
	buf[idx++] = crc >> 8;
	buf[idx++] = crc >> 0;
	
	MBS_StartSending(idx);
}
static void Config_ReadConfReq(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	uint16_t crc1 = Config_Crc16(buf, 6);
	uint16_t crc2 = ((uint16_t)buf[6]<<8) + ((uint16_t)buf[7]<<0);
	if(crc1 == crc2)
	{
		Config_SendConfStmToPc();
	}
}
static void Config_SendEcho(void)
{
	uint8_t* buf = pC->Mbs.bufwrite;
	MBS_ClrStr(buf, MBS_BUFMAX);
	uint32_t idx = 0;
	buf[idx++] = 249;
	buf[idx++] = 250;
	buf[idx++] = 251;
	buf[idx++] = 252;
	buf[idx++] = 253;
	buf[idx++] = (uint8_t)frameConfig_Echo;
	
	uint16_t crc = Config_Crc16(buf, idx);
	buf[idx++] = crc >> 8;
	buf[idx++] = crc >> 0;
	
	MBS_StartSending(idx);
}
static void Config_ReadConfPcToStm(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	uint16_t crc1 = Config_Crc16(buf, 610);
	uint16_t crc2 = ((uint16_t)buf[610]<<8) + ((uint16_t)buf[611]<<0);
	if(crc1 == crc2)
	{
		uint32_t idx = 6;
		
		pC->Ee.wData[EeAdd_configWasUploaded] 					= (uint16_t)EE_CONFIGWASUPLOADED;
		
		pC->Ee.wData[EeAdd_stmProt] 										= buf[idx++];

		pC->Ee.wData[EeAdd_mbrtuTimeout] 								= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbrtuDataSwap] 							= buf[idx++];
		pC->Ee.wData[EeAdd_mbrtuAddress] 								= buf[idx++];
		pC->Ee.wData[EeAdd_mbrtuBaudrate] 							= buf[idx++];
		pC->Ee.wData[EeAdd_mbrtuParity] 								= buf[idx++];
		
		pC->Ee.wData[EeAdd_mbtcpTimeout] 								= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbtcpDataSwap] 							= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpIP0] 										= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpIP1] 										= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpIP2] 										= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpIP3] 										= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpMask0] 									= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpMask1] 									= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpMask2] 									= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpMask3] 									= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpGateway0] 							= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpGateway1] 							= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpGateway2] 							= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpGateway3] 							= buf[idx++];
		pC->Ee.wData[EeAdd_mbtcpSerwerCons] 						= buf[idx++];

		pC->Ee.wData[EeAdd_mbtcpSendAckTimeoutHigh] 		= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbtcpSendAckTimeoutLow] 			= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbtcpConnectAckTimeoutHigh] 	= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbtcpConnectAckTimeoutLow] 	= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbtcpCloseAckTimeoutHigh] 		= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbtcpCloseAckTimeoutLow] 		= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		
		pC->Ee.wData[EeAdd_pfbusTimeout] 								= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_pfbusDataSwap] 							= buf[idx++];
		pC->Ee.wData[EeAdd_pfbusIdentNumber] 						= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_pfbusAdress] 								= buf[idx++];
		pC->Ee.wData[EeAdd_pfbusBaudrate] 							= buf[idx++];
		pC->Ee.wData[EeAdd_pfbusDPV1Enable] 						= buf[idx++];
		pC->Ee.wData[EeAdd_pfbusSyncSupported] 					= buf[idx++];
		pC->Ee.wData[EeAdd_pfbusFreezeSupported] 				= buf[idx++];
		pC->Ee.wData[EeAdd_pfbusFailSafeSupported] 			= buf[idx++];
		
		pC->Ee.wData[EeAdd_pfnetTimeout] 								= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_pfnetDataSwap] 							= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetVendorId] 							= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_pfnetDeviceId] 							= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_pfnetLengthNameOfStation] 		= buf[idx++];
		for(uint16_t i=0;i<240;i++)
			pC->Ee.wData[EeAdd_pfnetNameOfStation + i] 		= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetLengthTypeOfStation] 		= buf[idx++];
		for(uint16_t i=0;i<240;i++)
			pC->Ee.wData[EeAdd_pfnetTypeOfStation + i] 		= buf[idx++];
		for(uint16_t i=0;i<28;i++)
			pC->Ee.wData[EeAdd_pfnetDeviceType + i] 			= buf[idx++];
		for(uint16_t i=0;i<20;i++)
			pC->Ee.wData[EeAdd_pfnetOrderId + i] 					= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetIP0] 										= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetIP1] 										= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetIP2] 										= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetIP3] 										= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetMask0] 									= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetMask1] 									= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetMask2] 									= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetMask3] 									= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetGateway0] 							= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetGateway1] 							= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetGateway2] 							= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetGateway3] 							= buf[idx++];
		pC->Ee.wData[EeAdd_pfnetHardwareRevision] 			= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_pfnetSoftwareRevision1] 			= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_pfnetSoftwareRevision2] 			= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_pfnetSoftwareRevision3] 			= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_pfnetSoftwareRevisionPrefix] = buf[idx++];
		
		FLASH_Unlock();
		EE_Init();
		for(uint16_t i=0;i<EE_VARMAX;i++)
			EE_WriteVariable(pC->Ee.VirtAddVarTab[i], pC->Ee.wData[i]);
		FLASH_Lock();
		
		Config_SendEcho();
	}
}
static void Config_ReadTelemReq(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	uint16_t crc1 = Config_Crc16(buf, 6);
	uint16_t crc2 = ((uint16_t)buf[6]<<8) + ((uint16_t)buf[7]<<0);
	if(crc1 == crc2)
	{
		Config_SendTelemStmToPc();
	}
}
static void Config_ReadEcho(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	uint16_t crc1 = Config_Crc16(buf, 6);
	uint16_t crc2 = ((uint16_t)buf[6]<<8) + ((uint16_t)buf[7]<<0);
	if(crc1 == crc2)
	{
		Config_SendEcho();
	}
}
static void Config_ReadFrame(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	switch(buf[5])
	{
		case frameConfig_ConfReq:
			Config_ReadConfReq();
			break;
		case frameConfig_ConfPcToStm:
			Config_ReadConfPcToStm();
			break;
		case frameConfig_TelemReq:
			Config_ReadTelemReq();
			break;
		case frameConfig_Echo:
			Config_ReadEcho();
			break;
		default:
			break;
	}
}
// Funkcje ogólne **************************************
static void ReadFromUsart(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	if(buf[0] == 249 && buf[1] == 250 && buf[2] == 251 && buf[3] == 252 && buf[4] == 253)
	{
		Config_ReadFrame();
	}
	else if(buf[0] == pC->Mbs.address && pC->Mode.protocol == Prot_Mbrtu)
	{
		MBS_ReadRequest();
	}
	MBS_ReloadDmaToRead();
}
void USART1_IRQHandler(void)
{
	if((USART1->SR & USART_SR_IDLE) != RESET)
	{
		ReadFromUsart();
		char c = USART1->DR;
	}
	if((USART1->SR & USART_SR_TC) != RESET)
	{
		MBS_SetRead();
		USART1->CR1 &= ~USART_CR1_TCIE;
	}
}
void DMA2_Stream7_IRQHandler(void)
{
	if((DMA2->HISR & DMA_HISR_TCIF7) != RESET)
	{
		USART1->CR1 |= USART_CR1_TCIE;
		DMA2_Stream7->CR &= ~DMA_SxCR_EN;
		DMA2->HIFCR |= DMA_HIFCR_CTCIF7;
	}
}
