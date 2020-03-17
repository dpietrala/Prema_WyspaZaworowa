#include "MB_RTU_Slave.h"
sMBS MBS;
sMBS* pMBS = &MBS;
void MBS_Conf(void)
{
	MBS_ClrStr(pMBS->bufread, MBS_BUFMAX);
	MBS_ClrStr(pMBS->bufwrite, MBS_BUFMAX);
	pMBS->baud = 9600;
	pMBS->unittime = 1000000/pMBS->baud;
	pMBS->fun = MF_I;
	pMBS->address = 2;
	pMBS->coils = 0;
	for(uint16_t j=0;j<MBS_REGMAX;j++)
		pMBS->hregs[j] = 0;
	
	DMA1_Stream1->PAR 	= (uint32_t)&USART3->DR;
	DMA1_Stream1->M0AR 	= (uint32_t)pMBS->bufread;
	DMA1_Stream1->NDTR 	= (uint16_t)MBS_BUFMAX;
	DMA1_Stream1->CR 		|= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_EN;
	DMA1_Stream3->PAR 	= (uint32_t)&USART3->DR;
	DMA1_Stream3->M0AR 	= (uint32_t)pMBS->bufwrite;
	DMA1_Stream3->NDTR 	= (uint16_t)MBS_BUFMAX;
	DMA1_Stream3->CR 		|= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_DIR_0 | DMA_SxCR_TCIE;
	NVIC_EnableIRQ(DMA1_Stream3_IRQn);
	
	GPIOB->MODER |= GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1;
	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR10_0 | GPIO_PUPDR_PUPDR11_0;
	GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR10 | GPIO_OSPEEDER_OSPEEDR11;
	GPIOB->AFR[1] |= 0x00007700;
	USART3->BRR = 42000000/pMBS->baud;
	USART3->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;
	USART3->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE;
	NVIC_EnableIRQ(USART3_IRQn);
	
	TIM7->PSC = 84-1;
	TIM7->ARR = 10 * pMBS->unittime;
	TIM7->DIER |= TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM7_IRQn);
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

static void MBS_SetRead(void)
{
	LED4_OFF;
}
static void MBS_SetSend(void)
{
	LED4_ON;
}
static void MBS_ResponseError_IF(void)
{
}
static void MBS_ResponseError_IDR(void)
{
	pMBS->error = MFE_IDR;
	uint8_t* bufw = pMBS->bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	bufw[0] = pMBS->address;
	bufw[1] = 0x80 + pMBS->fun;
	bufw[2] = pMBS->error;
	uint16_t crc = MBS_Crc16(bufw, 3);
	bufw[3] = crc;
	bufw[4] = crc >> 8;

	DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	DMA1_Stream3->NDTR = 5;
	DMA1_Stream3->CR |= DMA_SxCR_EN;
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
	LED2_TOG;
	uint8_t* bufr = pMBS->bufread;
	uint8_t* bufw = pMBS->bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	uint16_t coilstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
	uint16_t coilnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
	uint8_t numbytes = (coilnum + 7) / 8; // wyznaczenie liczby bajtów. Zaokraglenie w góre
	
	uint16_t index = 0;
	bufw[index++] = pMBS->address;
	bufw[index++] = pMBS->fun;
	bufw[index++] = numbytes;
	
	if(numbytes == 1)
		bufw[index++] = (uint8_t)(pMBS->coils >> coilstart);
	if(numbytes == 2)
	{
		bufw[index++] = (uint8_t)(pMBS->coils >> coilstart);
		bufw[index++] = (uint8_t)(pMBS->coils >> (coilstart+8));
	}
	
	uint16_t crc = MBS_Crc16(bufw, index);
	bufw[index++] = crc;
	bufw[index++] = crc >> 8;
	
	DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	DMA1_Stream3->NDTR = index;
	DMA1_Stream3->CR |= DMA_SxCR_EN;
}
static void MBS_Response_WnDQ(void)
{
	uint8_t* bufr = pMBS->bufread;
	uint8_t* bufw = pMBS->bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	uint8_t numbytes = bufr[6];
	uint16_t coilstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
	uint16_t coilnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
	uint16_t coilend = coilstart + coilnum - 1;
	
	uint16_t temp1 = pMBS->coils;
	uint16_t temp2 = 0;
	if(numbytes == 1)
		temp2 = (uint16_t)bufr[7] << coilstart;
	if(numbytes == 2)
		temp2 = (((uint16_t)bufr[8]<<8) + ((uint16_t)bufr[7]<<0)) << coilstart;
	
	uint16_t mask = 0;
	for(int i=0;i<16;i++)
		if(i>=coilstart && i<= coilend)
			mask += (1<<i);
	
	pMBS->coils = ((temp1 & (~mask)) | (temp2 & mask));
		
	for(int i=0;i<6;i++)
		bufw[i] = bufr[i];
	
	uint16_t crc = MBS_Crc16(bufw, 6);
	bufw[6] = crc;
	bufw[7] = crc >> 8;
	
	DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	DMA1_Stream3->NDTR = 8;
	DMA1_Stream3->CR |= DMA_SxCR_EN;
}
static void MBS_Response_RnHR(void)
{
	uint8_t* bufr = pMBS->bufread;
	uint8_t* bufw = pMBS->bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	uint16_t regstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
	uint16_t regnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
	uint16_t regend = regstart + regnum - 1;
	uint16_t numbytes = 2 * regnum;
	
	uint16_t index = 0;
	bufw[index++] = pMBS->address;
	bufw[index++] = pMBS->fun;
	bufw[index++] = numbytes;
	for(int i=regstart;i<=regend;i++)
	{
		bufw[index++] = pMBS->hregs[i] >> 8;
		bufw[index++] = pMBS->hregs[i];
	}
	uint16_t crc = MBS_Crc16(bufw, index);
	bufw[index++] = crc;
	bufw[index++] = crc >> 8;
	
	DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	DMA1_Stream3->NDTR = index;
	DMA1_Stream3->CR |= DMA_SxCR_EN;
}
static void MBS_Response_WnHR(void)
{
	uint8_t* bufr = pMBS->bufread;
	uint8_t* bufw = pMBS->bufwrite;
	MBS_ClrStr(bufw, MBS_BUFMAX);
	uint16_t regstart = ((uint16_t)bufr[2]<<8) + ((uint16_t)bufr[3]<<0);
	uint16_t regnum = ((uint16_t)bufr[4]<<8) + ((uint16_t)bufr[5]<<0);
	uint16_t regend = regstart + regnum - 1;
	uint16_t index = 7;
	for(int i=regstart;i<=regend;i++)
	{
		pMBS->hregs[i] = ((uint16_t)bufr[index++]<<8);
		pMBS->hregs[i] += ((uint16_t)bufr[index++]<<0);
	}
	
	for(int i=0;i<6;i++)
		bufw[i] = bufr[i];
	uint16_t crc = MBS_Crc16(bufw, 6);
	bufw[6] = crc;
	bufw[7] = crc >> 8;
	
	DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	DMA1_Stream3->NDTR = 8;
	DMA1_Stream3->CR |= DMA_SxCR_EN;
}
static void MBS_ReadRequest_RnDQ(void)
{
	uint8_t* bufr = pMBS->bufread;
	uint16_t crc1 = MBS_Crc16(bufr, 6);
	uint16_t crc2 = ((uint16_t)bufr[6]<<0) + ((uint16_t)bufr[7]<<8);
	if(crc1 == crc2)
	{
		pMBS->fun = MF_RnDQ;
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
	uint8_t* bufr = pMBS->bufread;
	uint8_t numbytes = 7 + bufr[6];
	uint16_t crc1 = MBS_Crc16(bufr, numbytes);
	uint16_t crc2 = ((uint16_t)bufr[numbytes]<<0) + ((uint16_t)bufr[numbytes+1]<<8);
	if(crc1 == crc2)
	{
		pMBS->fun = MF_WnDQ;
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
	uint8_t* bufr = pMBS->bufread;
	uint16_t crc1 = MBS_Crc16(bufr, 6);
	uint16_t crc2 = ((uint16_t)bufr[6]<<0) + ((uint16_t)bufr[7]<<8);
	if(crc1 == crc2)
	{
		pMBS->fun = MF_RnHR;
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
	uint8_t* bufr = pMBS->bufread;
	uint8_t numbytes = 7 + bufr[6];
	uint16_t crc1 = MBS_Crc16(bufr, numbytes);
	uint16_t crc2 = ((uint16_t)bufr[numbytes]<<0) + ((uint16_t)bufr[numbytes+1]<<8);
	if(crc1 == crc2)
	{
		pMBS->fun = MF_WnHR;
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
	uint8_t* buf = pMBS->bufread;
	if(buf[0] == pMBS->address)
	{
		MBS_SetSend();
		TIM7->CR1 |= TIM_CR1_CEN;
	}
}
static void MBS_ReadRequest(void)
{
	uint8_t* buf = pMBS->bufread;
	if(buf[0] == pMBS->address)
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
	MBS_ClrStr(buf, MBS_BUFMAX);
	DMA1_Stream1->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF1;
	DMA1_Stream1->CR |= DMA_SxCR_EN;
}
void USART3_IRQHandler(void)
{
	if((USART3->SR & USART_SR_IDLE) != RESET)
	{
		MBS_CheckAddress();
		char c = USART3->DR;
	}
}
void DMA1_Stream3_IRQHandler(void)
{
	if((DMA1->LISR & DMA_LISR_TCIF3) != RESET)
	{
		MBS_SetRead();
		DMA1_Stream3->CR &= ~DMA_SxCR_EN;
		DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	}
}
void TIM7_IRQHandler(void)
{
	if((TIM7->SR & TIM_SR_UIF) != RESET)
	{
		TIM7->CR1 &= ~TIM_CR1_CEN;
		MBS_ReadRequest();
		TIM7->SR &= ~TIM_SR_UIF;
	}
}
