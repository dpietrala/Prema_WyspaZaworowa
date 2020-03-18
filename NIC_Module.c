#include "NIC_Module.h"
extern sControl* pC;
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
	pC->Nic.fun = MF_I;
	pC->Nic.timeout = 500;
	pC->Nic.address = 2;
}
void NIC_ClrStr(uint8_t* str, uint32_t n)
{
	for(uint32_t i=0;i<n;i++)
		str[i] = 0x00;
}
uint16_t NIC_Crc16(uint8_t* buf, uint32_t len)
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
void NIC_WriteRegs(uint16_t addr0, uint16_t numregs)
{
	uint8_t* buf = pC->Nic.bufwrite;
	NIC_ClrStr(buf, NIC_BUFMAX);
	
	addr0 += 2000;
	uint16_t index = 0;
	buf[index++] = pC->Nic.address;
	buf[index++] = MF_WnHR;
	buf[index++] = addr0 >> 8;
	buf[index++] = addr0;
	buf[index++] = numregs >> 8;
	buf[index++] = numregs;
	buf[index++] = 2*numregs;
	for(uint16_t i=0;i<numregs;i++)
	{
		buf[index++] = pC->Nic.hregs[addr0 - 2000 + i] >> 8;
		buf[index++] = pC->Nic.hregs[addr0 - 2000 + i];
	}
	uint16_t crc = NIC_Crc16(buf, index);
	buf[index++] = crc;
	buf[index++] = crc >> 8;
	
	DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	DMA1_Stream3->NDTR 	= index;
	DMA1_Stream3->CR |= DMA_SxCR_EN;
	
	pC->Nic.fun = MF_WnHR;
	pC->Nic.reg0 = addr0 - 2000;
	pC->Nic.numregs = numregs;
}
void NIC_ReadRegs(uint16_t addr0, uint16_t numregs)
{
	uint8_t* buf = pC->Nic.bufwrite;
	NIC_ClrStr(buf, NIC_BUFMAX);
	addr0 += 1000;
	uint16_t index = 0;
	buf[index++] = pC->Nic.address;
	buf[index++] = MF_RnHR;
	buf[index++] = addr0 >> 8;
	buf[index++] = addr0;
	buf[index++] = numregs >> 8;
	buf[index++] = numregs;
	
	uint16_t crc = NIC_Crc16(buf, index);
	buf[index++] = crc;
	buf[index++] = crc >> 8;
	
	DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	DMA1_Stream3->NDTR 	= index;
	DMA1_Stream3->CR |= DMA_SxCR_EN;
	
	pC->Nic.fun = MF_RnHR;
	pC->Nic.reg0 = addr0 - 1000;
	pC->Nic.numregs = numregs;
}
static void NIC_ReadRequestAfterWriteRegs(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t flag = 0;
	
	if(address == pC->Nic.address)
		flag = 1;
	if(flag == 0)
		return;
	if(buf[1] != pC->Nic.fun)
		return;
	
	uint16_t crc1 = NIC_Crc16(buf, 6);
	uint16_t crc2 = ((uint16_t)buf[6]<<0) + ((uint16_t)buf[7]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		pC->Nic.fun = MF_I;
		pC->Nic.reg0 = 0;
		pC->Nic.numregs = 0;
	}
	return;
}
static void NIC_ReadRequestAfterReadRegs(void)
{
	uint8_t* buf = pC->Nic.bufread;
	uint8_t address = buf[0];
	uint8_t fun = buf[1];
	uint8_t bytes = buf[2];
	uint8_t regs = bytes/2;
	uint8_t flag = 0;
	if(address == pC->Nic.address)
		flag = 1;
	if(flag == 0)
		return;
	
	if(fun != pC->Nic.fun)
		return;

	uint16_t crc1 = NIC_Crc16(buf, bytes+3);
	uint16_t crc2 = ((uint16_t)buf[bytes+3]<<0) + ((uint16_t)buf[bytes+3+1]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		for(uint8_t i=0;i<regs;i++)
		{
			uint16_t reg = ((uint16_t)buf[3+2*i+0]<<8) + ((uint16_t)buf[3+2*i+1]<<0);
			pC->Nic.hregs[pC->Nic.reg0 + i] = reg;
		}
		pC->Nic.fun = MF_I;
		pC->Nic.reg0 = 0;
		pC->Nic.numregs = 0;
	}
	return;
}
static void NIC_ReadRequestAfterFunctionError(void)
{
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
}
static void NIC_ReadReaquest(void)
{
	uint8_t* buf = pC->Nic.bufread;
	switch(buf[1])
	{
		case MF_I:
			break;
		case MF_WnHR:
			NIC_ReadRequestAfterWriteRegs();
			break;
		case MF_RnHR:
			NIC_ReadRequestAfterReadRegs();
			break;
		
//		case MFE_IF:
//			NIC_ReadRequestAfterFunctionError();
//			break;
//		case MFE_IDR:
//			NIC_ReadRequestAfterFunctionError();
//			break;
//		case MFE_IV:
//			NIC_ReadRequestAfterFunctionError();
//			break;
//		case MFE_SE:
//			NIC_ReadRequestAfterFunctionError();
//			break;
//		case MFE_PC:
//			NIC_ReadRequestAfterFunctionError();
//			break;
//		case MFE_SNR:
//			NIC_ReadRequestAfterFunctionError();
//			break;
//		case MFE_NC:
//			NIC_ReadRequestAfterFunctionError();
//			break;
//		case MFE_PE:
//			NIC_ReadRequestAfterFunctionError();
//			break;
		
		default:
			break;
	}
	NIC_ClrStr(buf, NIC_BUFMAX);
	DMA1_Stream1->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF1;
	DMA1_Stream1->CR |= DMA_SxCR_EN;
	return;
}
void USART3_IRQHandler(void)
{
	if((USART3->SR & USART_SR_IDLE) != RESET)
	{
		NIC_ReadReaquest();
		char c = USART3->DR;
	}
}
