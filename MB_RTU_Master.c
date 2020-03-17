#include "MB_RTU_Master.h"
sModbus Modbus;
sModbus* pModbus = &Modbus;
void Modbus_Conf(void)
{
	DMA1_Stream1->PAR 	= (uint32_t)&USART3->DR;
	DMA1_Stream1->M0AR 	= (uint32_t)pModbus->bufread;
	DMA1_Stream1->NDTR 	= (uint16_t)BUFMAX;
	DMA1_Stream1->CR 		|= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_EN;
	DMA1_Stream3->PAR 	= (uint32_t)&USART3->DR;
	DMA1_Stream3->M0AR 	= (uint32_t)pModbus->bufwrite;
	DMA1_Stream3->NDTR 	= (uint16_t)BUFMAX;
	DMA1_Stream3->CR 		|= DMA_SxCR_MINC | DMA_SxCR_CHSEL_2 | DMA_SxCR_DIR_0;
	GPIOB->MODER |= GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1;
	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR10_0 | GPIO_PUPDR_PUPDR11_0;
	GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR10 | GPIO_OSPEEDER_OSPEEDR11;
	GPIOB->AFR[1] |= 0x00007700;
	USART3->BRR = 42000000/115200;
	USART3->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;
	USART3->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE;
	NVIC_EnableIRQ(USART3_IRQn);
	
	Modbus_ClrStr(pModbus->bufread, BUFMAX);
	Modbus_ClrStr(pModbus->bufwrite, BUFMAX);
	pModbus->slave.state = MFI;
	pModbus->slave.timeout = 500;
	pModbus->slave.address = 2;
	for(uint16_t j=0;j<COILMAX;j++)
		pModbus->slave.coils[j] = 0;
	for(uint16_t j=0;j<REGMAX;j++)
		pModbus->slave.holdreg[j] = 0;
}
void Modbus_ClrStr(uint8_t* str, uint32_t n)
{
	for(uint32_t i=0;i<n;i++)
		str[i] = 0x00;
}
uint16_t Modbus_Crc16(uint8_t* buf, uint32_t len)
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
void Modbus_WriteRegs(uint16_t addr0, uint16_t numregs)
{
	uint8_t* buf = pModbus->bufwrite;
	Modbus_ClrStr(buf, BUFMAX);
	
	addr0 += 2000;
	uint16_t index = 0;
	buf[index++] = pModbus->slave.address;
	buf[index++] = MFWH;
	buf[index++] = addr0 >> 8;
	buf[index++] = addr0;
	buf[index++] = numregs >> 8;
	buf[index++] = numregs;
	buf[index++] = 2*numregs;
	for(uint16_t i=0;i<numregs;i++)
	{
		buf[index++] = pModbus->slave.holdreg[addr0 - 2000 + i] >> 8;
		buf[index++] = pModbus->slave.holdreg[addr0 - 2000 + i];
	}
	uint16_t crc = Modbus_Crc16(buf, index);
	buf[index++] = crc;
	buf[index++] = crc >> 8;
	
	DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	DMA1_Stream3->NDTR 	= index;
	DMA1_Stream3->CR |= DMA_SxCR_EN;
	
	pModbus->slave.state = MFWH;
	pModbus->slave.addr0 = addr0 - 2000;
	pModbus->slave.numregs = numregs;
}
void Modbus_ReadRegs(uint16_t addr0, uint16_t numregs)
{
	uint8_t* buf = pModbus->bufwrite;
	Modbus_ClrStr(buf, BUFMAX);
	addr0 += 1000;
	uint16_t index = 0;
	buf[index++] = pModbus->slave.address;
	buf[index++] = MFRH;
	buf[index++] = addr0 >> 8;
	buf[index++] = addr0;
	buf[index++] = numregs >> 8;
	buf[index++] = numregs;
	
	uint16_t crc = Modbus_Crc16(buf, index);
	buf[index++] = crc;
	buf[index++] = crc >> 8;
	
	DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF3;
	DMA1_Stream3->NDTR 	= index;
	DMA1_Stream3->CR |= DMA_SxCR_EN;
	
	pModbus->slave.state = MFRH;
	pModbus->slave.addr0 = addr0 - 1000;
	pModbus->slave.numregs = numregs;
}
static void Modbus_ReadRequestAfterWriteCoils(void)
{
}
static void Modbus_ReadRequestAfterReadCoils(void)
{
}
static void Modbus_ReadRequestAfterWriteRegs(void)
{
	uint8_t* buf = pModbus->bufread;
	uint8_t address = buf[0];
	uint8_t flag = 0;
	
	if(address == pModbus->slave.address)
		flag = 1;
	if(flag == 0)
		return;
	sSlave* pSlv = &pModbus->slave;
	if(buf[1] != pSlv->state)
		return;
	
	uint16_t crc1 = Modbus_Crc16(buf, 6);
	uint16_t crc2 = ((uint16_t)buf[6]<<0) + ((uint16_t)buf[7]<<8);
	if(crc1 != crc2)
	{
		return;
	}
	else
	{
		pSlv->state = MFI;
		pSlv->addr0 = 0;
		pSlv->numregs = 0;
	}
	return;
}
static void Modbus_ReadRequestAfterReadRegs(void)
{
	uint8_t* buf = pModbus->bufread;
	uint8_t address = buf[0];
	uint8_t fun = buf[1];
	uint8_t bytes = buf[2];
	uint8_t regs = bytes/2;
	uint8_t flag = 0;
	if(address == pModbus->slave.address)
		flag = 1;
	if(flag == 0)
		return;
	
	if(fun != pModbus->slave.state)
		return;

	uint16_t crc1 = Modbus_Crc16(buf, bytes+3);
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
			pModbus->slave.holdreg[pModbus->slave.addr0 + i] = reg;
		}
		pModbus->slave.state = MFI;
		pModbus->slave.addr0 = 0;
		pModbus->slave.numregs = 0;
	}
	return;
}
static void Modbus_ReadRequestAfterFunctionError(void)
{
//	uint8_t* buf = pModbus->bufread;
//	uint8_t address = buf[0];
//	uint8_t fun = buf[1];
//	uint8_t bytes = buf[2];
//	uint8_t regs = bytes/2;
//	uint8_t flag = 0;
//	for(uint8_t i=0;i<SLAVEMAX;i++)
//		if(address == pModbus->slaves[i].address)
//			flag = 1;
//	if(flag == 0)
//		return;
//	
//	if(fun != pModbus->slaves[address].state)
//		return;

//	uint16_t crc1 = Modbus_Crc16(buf, bytes+3);
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
//			pModbus->slaves[address].holdreg[pModbus->slaves[address].addr0 + i] = reg;
//		}
//		pModbus->slaves[address].state = MFI;
//		pModbus->slaves[address].addr0 = 0;
//		pModbus->slaves[address].numregs = 0;
//	}
//	return;
}
static void Modbus_ReadReaquest(void)
{
	uint8_t* buf = pModbus->bufread;
	switch(buf[1])
	{
		case MFI:
			break;
		case MFWC:
			Modbus_ReadRequestAfterWriteCoils();
			break;
		case MFRC:
			Modbus_ReadRequestAfterReadCoils();
			break;
		case MFWH:
			LED3_TOG;
			Modbus_ReadRequestAfterWriteRegs();
			break;
		case MFRH:
			LED2_TOG;
			Modbus_ReadRequestAfterReadRegs();
			break;
		
		case MFE_IF:
			Modbus_ReadRequestAfterFunctionError();
			break;
		case MFE_IDR:
			Modbus_ReadRequestAfterFunctionError();
			break;
		case MFE_IV:
			Modbus_ReadRequestAfterFunctionError();
			break;
		case MFE_SE:
			Modbus_ReadRequestAfterFunctionError();
			break;
		case MFE_PC:
			Modbus_ReadRequestAfterFunctionError();
			break;
		case MFE_SNR:
			Modbus_ReadRequestAfterFunctionError();
			break;
		case MFE_NC:
			Modbus_ReadRequestAfterFunctionError();
			break;
		case MFE_PE:
			Modbus_ReadRequestAfterFunctionError();
			break;
		
		default:
			break;
	}
//	Modbus_ClrStr(buf, BUFMAX);
	DMA1_Stream1->CR &= ~DMA_SxCR_EN;
	DMA1->LIFCR |= DMA_LIFCR_CTCIF1;
	DMA1_Stream1->CR |= DMA_SxCR_EN;
	return;
}
void USART3_IRQHandler(void)
{
	if((USART3->SR & USART_SR_IDLE) != RESET)
	{
		Modbus_ReadReaquest();
		char c = USART3->DR;
	}
}
