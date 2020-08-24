#include "MB_RTU_Slave.h"
extern sControl* pC;
static void MBS_SetSend(void)
{
	GPIOA->ODR |= GPIO_ODR_OD11;
}
static void MBS_SetRead(void)
{
	GPIOA->ODR &= ~GPIO_ODR_OD11;
}
eResult MBS_ComConf(void)
{
	eResult result = RES_OK;
	
	MBS_ClrStr(pC->Mbs.bufread, MBS_BUFMAX);
	MBS_ClrStr(pC->Mbs.bufwrite, MBS_BUFMAX);
	
	DMA2_Stream2->PAR 	= (uint32_t)&USART1->DR;
	DMA2_Stream2->M0AR 	= (uint32_t)pC->Mbs.bufread;
	DMA2_Stream2->NDTR 	= (uint16_t)MBS_BUFMAX;
	DMA2_Stream2->CR 		|= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_EN;
	DMA2_Stream7->PAR 	= (uint32_t)&USART1->DR;
	DMA2_Stream7->M0AR 	= (uint32_t)pC->Mbs.bufwrite;
	DMA2_Stream7->NDTR 	= (uint16_t)MBS_BUFMAX;
	DMA2_Stream7->CR 		|= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_DIR_0 | DMA_SxCR_TCIE;
	NVIC_EnableIRQ(DMA2_Stream7_IRQn);
	
	GPIOA->MODER |= GPIO_MODER_MODER11_0;
	MBS_SetRead();
	
	GPIOA->MODER |= GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR10_0;
	GPIOA->AFR[1] = 0x00000770;
	USART1->BRR = 50000000/9600;
	USART1->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;
	USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE;
	NVIC_EnableIRQ(USART1_IRQn);
	
	return result;
}
static void MBS_ReloadDmaToSend(uint16_t num)
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
static void MBS_ResponseError_IF(void)
{
}
static void MBS_ResponseError_IDR(void)
{
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

	MBS_ReloadDmaToSend(idx);
}
static void MBS_ResponseError_IV(void)
{
}
static void MBS_ResponseError_SE(void)
{
}
static void MBS_ResponseError_PC(void)
{
}
static void MBS_ResponseError_SNR(void)
{
}
static void MBS_ResponseError_NC(void)
{
}
static void MBS_ResponseError_PE(void)
{
}
static void MBS_Response_RnDQ(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	uint8_t* bufw = pC->Mbs.bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	uint16_t coilstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
	uint16_t coilnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
	uint8_t numbytes = (coilnum + 7) / 8; // wyznaczenie liczby bajtów. Zaokraglenie w góre
	
	uint16_t index = 0;
	bufw[index++] = pC->Mbs.address;
	bufw[index++] = pC->Mbs.fun;
	bufw[index++] = numbytes;
	
	if(numbytes == 1)
		bufw[index++] = (uint8_t)(pC->Mbs.coils >> coilstart);
	if(numbytes == 2)
	{
		bufw[index++] = (uint8_t)(pC->Mbs.coils >> coilstart);
		bufw[index++] = (uint8_t)(pC->Mbs.coils >> (coilstart+8));
	}
	
	uint16_t crc = MBS_Crc16(bufw, index);
	bufw[index++] = crc;
	bufw[index++] = crc >> 8;
	
	MBS_ReloadDmaToSend(index);
}
static void MBS_Response_WnDQ(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	uint8_t* bufw = pC->Mbs.bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	uint8_t numbytes = bufr[6];
	uint16_t coilstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
	uint16_t coilnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
	uint16_t coilend = coilstart + coilnum - 1;
	
	uint16_t temp1 = pC->Mbs.coils;
	uint16_t temp2 = 0;
	if(numbytes == 1)
		temp2 = (uint16_t)bufr[7] << coilstart;
	if(numbytes == 2)
		temp2 = (((uint16_t)bufr[8]<<8) + ((uint16_t)bufr[7]<<0)) << coilstart;
	
	uint16_t mask = 0;
	for(int i=0;i<16;i++)
		if(i>=coilstart && i<= coilend)
			mask += (1<<i);
	
	pC->Mbs.coils = ((temp1 & (~mask)) | (temp2 & mask));
		
	for(int i=0;i<6;i++)
		bufw[i] = bufr[i];
	
	uint16_t crc = MBS_Crc16(bufw, 6);
	bufw[6] = crc;
	bufw[7] = crc >> 8;
	
	MBS_ReloadDmaToSend(8);
}
static void MBS_Response_RnHR(void)
{
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
		bufw[index++] = pC->Mbs.hregs[i] >> 8;
		bufw[index++] = pC->Mbs.hregs[i];
	}
	uint16_t crc = MBS_Crc16(bufw, index);
	bufw[index++] = crc;
	bufw[index++] = crc >> 8;
	
	MBS_ReloadDmaToSend(index);
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
		pC->Mbs.hregs[i] = ((uint16_t)bufr[index++]<<8);
		pC->Mbs.hregs[i] += ((uint16_t)bufr[index++]<<0);
	}
	
	for(int i=0;i<6;i++)
		bufw[i] = bufr[i];
	uint16_t crc = MBS_Crc16(bufw, 6);
	bufw[6] = crc;
	bufw[7] = crc >> 8;
	
	MBS_ReloadDmaToSend(8);
}
static void MBS_ReadRequest_RnDQ(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	uint16_t crc1 = MBS_Crc16(bufr, 6);
	uint16_t crc2 = ((uint16_t)bufr[6]<<0) + ((uint16_t)bufr[7]<<8);
	if(crc1 == crc2)
	{
		pC->Mbs.fun = MF_RnDQ;
		uint16_t coilstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
		uint16_t coilnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
		uint16_t coilend = coilstart + coilnum - 1;
		if(coilend >= MBS_COILMAX)
		{
			MBS_ResponseError_IDR();
		}
		else
		{
			MBS_Response_RnDQ();
		}
	}
}
static void MBS_ReadRequest_WnDQ(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	uint8_t numbytes = 7 + bufr[6];
	uint16_t crc1 = MBS_Crc16(bufr, numbytes);
	uint16_t crc2 = ((uint16_t)bufr[numbytes]<<0) + ((uint16_t)bufr[numbytes+1]<<8);
	if(crc1 == crc2)
	{
		pC->Mbs.fun = MF_WnDQ;
		uint16_t coilstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
		uint16_t coilnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
		uint16_t coilend = coilstart + coilnum - 1;
		if(coilend >= MBS_COILMAX)
		{
			MBS_ResponseError_IDR();
		}
		else
		{
			MBS_Response_WnDQ();
		}
	}
}
static void MBS_ReadRequest_RnHR(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	uint16_t crc1 = MBS_Crc16(bufr, 6);
	uint16_t crc2 = ((uint16_t)bufr[6]<<0) + ((uint16_t)bufr[7]<<8);
	if(crc1 == crc2)
	{
		pC->Mbs.fun = MF_RnHR;
		uint16_t regstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
		uint16_t regnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
		uint16_t regend = regstart + regnum - 1;
		if(regend >= MBS_REGMAX)
		{
			MBS_ResponseError_IDR();
		}
		else
		{
			MBS_Response_RnHR();
		}
	}
}
static void MBS_ReadRequest_WnHR(void)
{
	uint8_t* bufr = pC->Mbs.bufread;
	uint8_t numbytes = 7 + bufr[6];
	uint16_t crc1 = MBS_Crc16(bufr, numbytes);
	uint16_t crc2 = ((uint16_t)bufr[numbytes]<<0) + ((uint16_t)bufr[numbytes+1]<<8);
	if(crc1 == crc2)
	{
		pC->Mbs.fun = MF_WnHR;
		uint16_t regstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
		uint16_t regnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
		uint16_t regend = regstart + regnum - 1;
		if(regend >= MBS_REGMAX)
		{
			MBS_ResponseError_IDR();
		}
		else
		{
			MBS_Response_WnHR();
		}
	}
}
static void MBS_CheckAddress(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	if(buf[0] == pC->Mbs.address)
	{
		MBS_SetSend();
	}
}
static void MBS_ReadRequest(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	if(buf[0] == pC->Mbs.address)
	{
		switch(buf[1])
		{
			case MF_RnDQ:		MBS_ReadRequest_RnDQ();	break;
			case MF_WnDQ:		MBS_ReadRequest_WnDQ(); break;
			case MF_RnHR:		MBS_ReadRequest_RnHR();	break;
			case MF_WnHR:		MBS_ReadRequest_WnHR();	break;
			default:																break;
		}
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
void Config_SendConfStmToPc(void)
{
	uint8_t* buf = pC->Mbs.bufwrite;
	MBS_ClrStr(buf, MBS_BUFMAX);
	uint32_t idx = 0;
	buf[idx++] = 247;
	buf[idx++] = 248;
	buf[idx++] = 249;
	buf[idx++] = 250;
	buf[idx++] = 251;
	buf[idx++] = (uint8_t)frameConfig_ConfStmToPc;
	
	buf[idx++] = pC->Ee.rData[EeAdd_stmProt] >> 0;
	
	buf[idx++] = pC->Ee.rData[EeAdd_mbrtuTimeout] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_mbrtuTimeout] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbrtuAddress] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_mbrtuBaudrate] >> 0;
	
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
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetSoftwareRevisionPrefix] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetSoftwareRevisionPrefix] >> 0;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetInstanceId] >> 8;
	buf[idx++] = pC->Ee.rData[EeAdd_pfnetInstanceId] >> 0;
	
	uint16_t crc = Config_Crc16(buf, idx);
	buf[idx++] = crc >> 8;
	buf[idx++] = crc >> 0;
	
	MBS_ReloadDmaToSend(idx + 2); //bo konwerter USB RS485 obcina dwa bajty ostatnie
}
static void Config_SendTelemStmToPc(void)
{
	uint8_t* buf = pC->Mbs.bufwrite;
	MBS_ClrStr(buf, MBS_BUFMAX);
	uint32_t idx = 0;
	buf[idx++] = 247;
	buf[idx++] = 248;
	buf[idx++] = 249;
	buf[idx++] = 250;
	buf[idx++] = 251;
	buf[idx++] = (uint8_t)frameConfig_TelemStmToPc;
	
	buf[idx++] = pC->Outs.coils >> 8;
	buf[idx++] = pC->Outs.coils >> 0;
	buf[idx++] = pC->Mode.workType >> 0;
	buf[idx++] = pC->Mode.protocol >> 0;
	buf[idx++] = (uint16_t)((int16_t)(pC->Mode.mcuTemp * 10)) >> 8;
	buf[idx++] = (uint16_t)((int16_t)(pC->Mode.mcuTemp * 10)) >> 0;
	buf[idx++] = pC->Status.status >> 24;
	buf[idx++] = pC->Status.status >> 16;
	buf[idx++] = pC->Status.status >> 8;
	buf[idx++] = pC->Status.status >> 0;
	
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
	buf[idx++] = pC->Nic.sscef.systemError >> 24;
	buf[idx++] = pC->Nic.sscef.systemError >> 16;
	buf[idx++] = pC->Nic.sscef.systemError >> 8;
	buf[idx++] = pC->Nic.sscef.systemError >> 0;
	buf[idx++] = pC->Nic.sscef.errorCounter >> 8;
	buf[idx++] = pC->Nic.sscef.errorCounter >> 0;
	buf[idx++] = pC->Nic.sscef.comError >> 24;
	buf[idx++] = pC->Nic.sscef.comError >> 16;
	buf[idx++] = pC->Nic.sscef.comError >> 8;
	buf[idx++] = pC->Nic.sscef.comError >> 0;
	buf[idx++] = pC->Nic.sscef.comStatus >> 24;
	buf[idx++] = pC->Nic.sscef.comStatus >> 16;
	buf[idx++] = pC->Nic.sscef.comStatus >> 8;
	buf[idx++] = pC->Nic.sscef.comStatus >> 0;
	buf[idx++] = pC->Nic.sscef.flagsSystem >> 8;
	buf[idx++] = pC->Nic.sscef.flagsSystem >> 0;
	
	uint16_t crc = Config_Crc16(buf, idx);
	buf[idx++] = crc >> 8;
	buf[idx++] = crc >> 0;
	pC->flaga5 = idx;
	
	MBS_ReloadDmaToSend(idx + 2); //bo konwerter USB RS485 obcina dwa bajty ostatnie
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
static void Config_ReadConfPcToStm(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	uint16_t crc1 = Config_Crc16(buf, 608);
	uint16_t crc2 = ((uint16_t)buf[608]<<8) + ((uint16_t)buf[609]<<0);
	if(crc1 == crc2)
	{
		uint32_t idx = 6;
		pC->Ee.wData[EeAdd_stmProt] 										= buf[idx++];

		pC->Ee.wData[EeAdd_mbrtuTimeout] 								= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbrtuAddress] 								= buf[idx++];
		pC->Ee.wData[EeAdd_mbrtuBaudrate] 							= buf[idx++];
		
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
		pC->Ee.wData[EeAdd_mbtcpSendAckTimeoutLow] 			= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbtcpSendAckTimeoutHigh] 		= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbtcpConnectAckTimeoutLow] 	= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbtcpConnectAckTimeoutHigh] 	= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbtcpCloseAckTimeoutLow] 		= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		pC->Ee.wData[EeAdd_mbtcpCloseAckTimeoutHigh] 		= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		
		pC->Ee.wData[EeAdd_pfbusTimeout] 								= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
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
		for(uint16_t i=0;i<240;i++)
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
		pC->Ee.wData[EeAdd_pfnetInstanceId] 						= ((uint16_t)buf[idx + 0] << 8) + ((uint16_t)buf[idx + 1] << 0);
		idx += 2;
		
		Control_WriteConfigToFlash();
	}
}
static void Config_ReadTelemReq(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	uint16_t crc1 = Config_Crc16(buf, 6);
	uint16_t crc2 = ((uint16_t)buf[6]<<8) + ((uint16_t)buf[7]<<0);
	//if(crc1 == crc2)
	{
		Config_SendTelemStmToPc();
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
		default:
			break;
	}
}
// Funkcje ogólne **************************************
static void ReadFromUsart(void)
{
	uint8_t* buf = pC->Mbs.bufread;
	if(buf[0] == 247 && buf[1] == 248 && buf[2] == 249 && buf[3] == 250 && buf[4] == 251)
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
}
void DMA2_Stream7_IRQHandler(void)
{
	if((DMA2->HISR & DMA_HISR_TCIF7) != RESET)
	{
		MBS_SetRead();
		DMA2_Stream7->CR &= ~DMA_SxCR_EN;
		DMA2->HIFCR |= DMA_HIFCR_CTCIF7;
	}
}
