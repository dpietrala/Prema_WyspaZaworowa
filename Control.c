#include "Control.h"
extern sControl* pC;
eResult result = RES_OK;
static void Control_RccSystemInit(void)
{
	uint32_t PLL_M=8, PLL_N=200, PLL_P=2, PLL_Q=7;
	RCC->CR |= RCC_CR_HSEON;								//W³¹czenie HSE
	while(!(RCC->CR & RCC_CR_HSERDY));						//czekamy a¿ HSE bêdzie gotowy
	RCC->CFGR = RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_HPRE_DIV1;
	RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1)-1) << 16) | (RCC_PLLCFGR_PLLSRC_HSE) | (PLL_Q << 24);
	FLASH->ACR |= FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;
	RCC->CR |= RCC_CR_PLLON;								//w³¹czenie PLL
	while(!(RCC->CR & RCC_CR_PLLRDY));						//czekanie a¿ PLL gotowa
	RCC->CFGR |= RCC_CFGR_SW_PLL;							//PLL jako Ÿród³o dla SYSCLK
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL); //czekanie a¿ PLL bedzie gotowe jako SYSCLK
}
static void Control_RccConf(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_TIM6EN;
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_ADC1EN;
}
static void Control_LedConf(void)
{
	LED_PORT->MODER 	|= GPIO_MODER_MODER8_0;
	LED_PORT->PUPDR 	|= GPIO_PUPDR_PUPDR8_0;
	LED1_OFF;
}
static void Control_StructConf(void)
{
//	pC->Mode.protocol = Prot_Pfnet;
	pC->Mode.workType = workTypeStop;
	
	for(uint32_t i=0;i<EE_VARMAX;i++)
		pC->Ee.VirtAddVarTab[i] = i;
	
	pC->Mode.ledTime = 0;
	pC->Mode.ledPeriod = 0;
	
	pC->Nic.mode.nicFun = NF_I;
	pC->Nic.mode.confStatus = NCS_confIsntDone;
	pC->Nic.mode.comStatus = NCS_comIsIdle;
	pC->Nic.mode.address = 2;
	pC->Nic.mode.comTime = 0;
	pC->Nic.mode.comTimeout = 100;
	pC->Nic.mode.flagTime = 0;
	pC->Nic.mode.flagTimeout = 1000;
	pC->Nic.mode.numFunToSend = 0;
	pC->Nic.mode.maxFunToSend = 0;
	
	NIC_SetDefaultSystemInformationMb();
	NIC_SetDefaultConfigurationMb();
	NIC_SetDefaultSystemInformationPfbus();
	NIC_SetDefaultConfigurationPfbus();
	NIC_SetDefaultSystemInformationPfnet();
	NIC_SetDefaultConfigurationPfnet();
}
static void Control_AdcConf(void)
{
	DMA2_Stream0->PAR = (uint32_t)&ADC1->DR;
	DMA2_Stream0->M0AR = (uint32_t)pC->Mode.adcValue;
	DMA2_Stream0->NDTR = (uint16_t)(200);
	DMA2_Stream0->CR |= DMA_SxCR_PSIZE_0 | DMA_SxCR_MSIZE_0 | DMA_SxCR_MINC | DMA_SxCR_CIRC | DMA_SxCR_EN;
	
	ADC->CCR 	|= ADC_CCR_ADCPRE | ADC_CCR_TSVREFE;
	ADC1->CR2 |= ADC_CR2_ADON | ADC_CR2_CONT | ADC_CR2_DDS | ADC_CR2_DMA;
	ADC1->CR1 |= ADC_CR1_SCAN;
	
	ADC1->SMPR1 |= ADC_SMPR1_SMP18;
	ADC1->SQR3 |= (18<<0);
	ADC1->CR2 |= ADC_CR2_SWSTART;
}
static void Control_ReadConfigFromFlash(void)
{
	FLASH_Unlock();
	EE_Init();
	for(uint16_t i=0;i<EE_VARMAX;i++)
		EE_ReadVariable(pC->Ee.VirtAddVarTab[i], &pC->Ee.rData[i]);
	FLASH_Lock();
	pC->Mode.protocol = (eProtocol)pC->Ee.rData[EeAdd_stmProt];
}
static void Control_WriteConfigToFlash(void)
{
	pC->Ee.wData[EeAdd_stmProt] 					= (uint16_t)Prot_Pfnet;
	
	pC->Ee.wData[EeAdd_mbrtuAddress] 			= 2;
	pC->Ee.wData[EeAdd_mbrtuTimeout] 			= 1000;
	pC->Ee.wData[EeAdd_mbrtuBaudrate] 		= 7;
	
	pC->Ee.wData[EeAdd_mbtcpTimeout] 			= 1000;
	pC->Ee.wData[EeAdd_mbtcpDataSwap] 		= 1;
	pC->Ee.wData[EeAdd_mbtcpIP0] 					= 14;
	pC->Ee.wData[EeAdd_mbtcpIP1] 					= 0;
	pC->Ee.wData[EeAdd_mbtcpIP2] 					= 168;
	pC->Ee.wData[EeAdd_mbtcpIP3] 					= 192;
	pC->Ee.wData[EeAdd_mbtcpMask0] 				= 0;
	pC->Ee.wData[EeAdd_mbtcpMask1] 				= 255;
	pC->Ee.wData[EeAdd_mbtcpMask2] 				= 255;
	pC->Ee.wData[EeAdd_mbtcpMask3] 				= 255;
	pC->Ee.wData[EeAdd_mbtcpGateway0] 		= 1;
	pC->Ee.wData[EeAdd_mbtcpGateway1] 		= 0;
	pC->Ee.wData[EeAdd_mbtcpGateway2] 		= 168;
	pC->Ee.wData[EeAdd_mbtcpGateway3] 		= 192;
	pC->Ee.wData[EeAdd_mbtcpSerwerCons] 	= 4;
	pC->Ee.wData[EeAdd_mbtcpSendAckTimeoutLow] 			= (uint16_t)31000;
	pC->Ee.wData[EeAdd_mbtcpSendAckTimeoutHigh] 		= (uint16_t)(31000 >> 16);
	pC->Ee.wData[EeAdd_mbtcpConnectAckTimeoutLow] 	= (uint16_t)31000;
	pC->Ee.wData[EeAdd_mbtcpConnectAckTimeoutHigh] 	= (uint16_t)(31000 >> 16);
	pC->Ee.wData[EeAdd_mbtcpCloseAckTimeoutLow] 		= (uint16_t)13000;
	pC->Ee.wData[EeAdd_mbtcpCloseAckTimeoutHigh] 		= (uint16_t)(13000 >> 16);
	
	pC->Ee.wData[EeAdd_pfbusTimeout] 								= 1000;
	pC->Ee.wData[EeAdd_pfbusAdress] 								= 3;
	pC->Ee.wData[EeAdd_pfbusBaudrate] 							= 15;
	pC->Ee.wData[EeAdd_pfbusDPV1Enable] 						= 1;
	pC->Ee.wData[EeAdd_pfbusSyncSupported] 					= 1;
	pC->Ee.wData[EeAdd_pfbusFreezeSupported] 				= 1;
	pC->Ee.wData[EeAdd_pfbusFailSafeSupported] 			= 1;
	
	pC->Ee.wData[EeAdd_pfnetTimeout] 		= 1000;
	pC->Ee.wData[EeAdd_pfnetVendorId] 	= 1;
	pC->Ee.wData[EeAdd_pfnetDeviceId] 	= 1;
	pC->Ee.wData[EeAdd_pfnetIP0] 				= 19;
	pC->Ee.wData[EeAdd_pfnetIP1] 				= 0;
	pC->Ee.wData[EeAdd_pfnetIP2] 				= 168;
	pC->Ee.wData[EeAdd_pfnetIP3] 				= 192;
	pC->Ee.wData[EeAdd_pfnetMask0] 			= 0;
	pC->Ee.wData[EeAdd_pfnetMask1] 			= 255;
	pC->Ee.wData[EeAdd_pfnetMask2] 			= 255;
	pC->Ee.wData[EeAdd_pfnetMask3] 			= 255;
	pC->Ee.wData[EeAdd_pfnetGateway0] 	= 1;
	pC->Ee.wData[EeAdd_pfnetGateway1] 	= 0;
	pC->Ee.wData[EeAdd_pfnetGateway2] 	= 168;
	pC->Ee.wData[EeAdd_pfnetGateway3] 	= 192;
	
	
	FLASH_Unlock();
	EE_Init();
	for(uint16_t i=0;i<EE_VARMAX;i++)
		EE_WriteVariable(pC->Ee.VirtAddVarTab[i], pC->Ee.wData[i]);
	FLASH_Lock();
}
static void Control_TimsConf(void)
{
	TIM6->PSC = 50-1;
	TIM6->ARR = 10000;
	TIM6->DIER |= TIM_DIER_UIE;
	TIM6->CR1 |= TIM_CR1_CEN;
	NVIC_EnableIRQ(TIM6_DAC_IRQn);
}
void Control_SystemInit(void)
{
	Control_RccSystemInit();
	SysTick_Config(100000);
	Control_RccConf();
	Control_LedConf();
	Control_StructConf();
	Control_AdcConf();
	Control_WriteConfigToFlash();
	Control_ReadConfigFromFlash();
}
void Control_SystemStart(void)
{
	Control_TimsConf();
}
static void Control_LedAct(void)
{
	if(pC->Mode.workType == workTypeStop)
	{
		LED1_OFF;
		pC->Mode.ledTime = 0;
	}
	else if(pC->Mode.workType == workTypeConf)
	{
		LED1_ON;
		pC->Mode.ledTime = 0;
	}
	else if(pC->Mode.ledTime >= pC->Mode.ledPeriod)
	{
		LED1_TOG;
		pC->Mode.ledTime = 0;
	}
}
void delay_ms(uint32_t ms)
{
	pC->Mode.tick = 0;
	while(pC->Mode.tick < ms);
}
static void Control_CheckStatus(void)
{
	if(pC->Nic.mode.flagWaitingForFlag == true)
		pC->Nic.mode.flagTime++;
	if(pC->Nic.mode.comStatus != NCS_comIsIdle && pC->Nic.mode.comStatus != NCS_comIsDone)
		pC->Nic.mode.comTime++;
	
	if(pC->Nic.mode.comTime >= pC->Nic.mode.comTimeout)
	{
		pC->Status.nicComTimeoutError = true;
	}
	else
	{
		pC->Status.nicComTimeoutError = false;
	}
	if(pC->Nic.mode.flagTime >= pC->Nic.mode.flagTimeout)
	{
		pC->Status.nicFlagTimeoutError = true;
	}
	else
	{
		pC->Status.nicFlagTimeoutError = false;
	}
}
void SysTick_Handler(void)
{
	pC->Mode.tick++;
	pC->Mode.ledTime++;
	Control_CheckStatus();
}
static void Control_ReadMcuTemp(void)
{
	uint32_t suma = 0;
	for(uint16_t i=0;i<200;i++)
		suma += pC->Mode.adcValue[i];
	
	double VSENSE = (double)suma / 200.0 / 4096.0 * 3.3;
	double V25 = 0.76;//V
	double Avg_Slope = 0.0025; //V/C
	pC->Mode.mcuTemp = ((VSENSE - V25) / Avg_Slope) + 25.0;
}
//*******************************************************************************
static eResult Control_WaitForNicComunicationIsDone(uint32_t* time)
{
	eResult result = RES_OK;
	while(pC->Nic.mode.comStatus != NCS_comIsDone)
	{
		if(pC->Status.nicComTimeoutError == true)
		{
			pC->Nic.mode.comStatus = NCS_comIsIdle;
			result = RES_NicComTimeout;
			return result;
		}
	}
	*time = pC->Nic.mode.comTime;
	return result;
}
static eResult Control_WaitForNicFlagIsChanged(eBool flag, eBool newstate, uint32_t timeout)
{
	eResult result = RES_OK;
	pC->Nic.mode.flagTime = 0;
	pC->Nic.mode.flagTimeout = timeout;
	pC->Nic.mode.flagWaitingForFlag = true;
	while(flag != newstate)
	{
		pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemStatusComErrorFlags;
		NIC_StartComunication(1, 10000);
		result = Control_WaitForNicComunicationIsDone(&pC->time0);
		if(result != RES_OK)
		{
			pC->Nic.mode.flagWaitingForFlag = false;
			return result;
		}
		if(pC->Status.nicFlagTimeoutError == true)
		{
			pC->Nic.mode.flagWaitingForFlag = false;
			result = RES_NicFlagTimeout;
			return result;
		}
	}
	pC->Nic.mode.flagWaitingForFlag = false;
	return result;
}
static eResult Control_ReadModuleSystemInformation(void)
{
	eResult result = RES_OK;
	pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemInformation;
	NIC_StartComunication(1, 10000);
	result = Control_WaitForNicComunicationIsDone(&pC->time0);
	return result;
}
// Modbus RTU ************************************************
static eResult Control_ConfModbusRTU(void)
{
	eResult result = RES_OK;
	pC->Mbs.address = (uint8_t)pC->Ee.rData[EeAdd_mbrtuAddress];
	pC->Mbs.timeout	= pC->Ee.rData[EeAdd_mbrtuTimeout];
	switch(pC->Ee.rData[EeAdd_mbrtuBaudrate])
	{
		case 0:		pC->Mbs.baud = 1200;		break;
		case 1:		pC->Mbs.baud = 2400;		break;
		case 2:		pC->Mbs.baud = 4800;		break;
		case 3:		pC->Mbs.baud = 9600;		break;
		case 4:		pC->Mbs.baud = 19200;		break;
		case 5:		pC->Mbs.baud = 38400;		break;
		case 6:		pC->Mbs.baud = 57600;		break;
		case 7:		pC->Mbs.baud = 115200;	break;
		default: 	pC->Mbs.baud = 9600;		break;
	}
	return result;
}
static void Control_RunModbusRTU(void)
{
}
// Modbus TCP ************************************************
static eResult Control_ComapreSystemInformationModbusTCP(void)
{
	eResult result = RES_OK;

	if(pC->Nic.si.devNumber != pC->Nic.siDefMb.devNumber)
	{
		result = RES_NicSiIncompatible;
		pC->Status.nicIncompDevNumber = true;
	}
	
	if(pC->Nic.si.devClass 	!= pC->Nic.siDefMb.devClass)
	{
		result = RES_NicSiIncompatible;
		pC->Status.nicIncompDevClass = true;
	}
	
	if(pC->Nic.si.protClass != pC->Nic.siDefMb.protClass)
	{
		result = RES_NicSiIncompatible;
		pC->Status.nicIncompProtClass = true;
	}
	
	for(uint8_t i=0;i<64;i++)
		if(pC->Nic.si.firmName[i] != pC->Nic.siDefMb.firmName[i])
		{
			result = RES_NicSiIncompatible;
			pC->Status.nicIncompFirmName = true;
		}
	
	return result;
}
static eResult Control_ReadConfigurationFromModuleModbusTCP(void)
{
	eResult result = RES_OK;
	pC->Nic.mode.tabFunToSend[0] = NIC_ReadNetworkConfigurationMb300_332;
	NIC_StartComunication(1, 10000);
	result = Control_WaitForNicComunicationIsDone(&pC->time0);
	return result;
}
static void Control_PrepareConfigurationToWriteModbusTCP(void)
{
	pC->Nic.ncMbWrite = pC->Nic.ncMbDef;
	
	pC->Nic.ncMbWrite.wdgTimeout = pC->Ee.rData[EeAdd_mbtcpTimeout];
	pC->Nic.ncMbWrite.dataSwap = pC->Ee.rData[EeAdd_mbtcpDataSwap];
	pC->Nic.ncMbWrite.ipAddress[0] = pC->Ee.rData[EeAdd_mbtcpIP0];
	pC->Nic.ncMbWrite.ipAddress[1] = pC->Ee.rData[EeAdd_mbtcpIP1];
	pC->Nic.ncMbWrite.ipAddress[2] = pC->Ee.rData[EeAdd_mbtcpIP2];
	pC->Nic.ncMbWrite.ipAddress[3] = pC->Ee.rData[EeAdd_mbtcpIP3];
	pC->Nic.ncMbWrite.subnetMask[0] = pC->Ee.rData[EeAdd_mbtcpMask0];
	pC->Nic.ncMbWrite.subnetMask[1] = pC->Ee.rData[EeAdd_mbtcpMask1];
	pC->Nic.ncMbWrite.subnetMask[2] = pC->Ee.rData[EeAdd_mbtcpMask2];
	pC->Nic.ncMbWrite.subnetMask[3] = pC->Ee.rData[EeAdd_mbtcpMask3];
	pC->Nic.ncMbWrite.gateway[0] = pC->Ee.rData[EeAdd_mbtcpGateway0];
	pC->Nic.ncMbWrite.gateway[1] = pC->Ee.rData[EeAdd_mbtcpGateway1];
	pC->Nic.ncMbWrite.gateway[2] = pC->Ee.rData[EeAdd_mbtcpGateway2];
	pC->Nic.ncMbWrite.gateway[3] = pC->Ee.rData[EeAdd_mbtcpGateway3];
	pC->Nic.ncMbWrite.provSerwerConn = pC->Ee.rData[EeAdd_mbtcpSerwerCons];
	pC->Nic.ncMbWrite.sendAckTimeout = ((uint32_t)pC->Ee.rData[EeAdd_mbtcpSendAckTimeoutHigh] << 16) + ((uint32_t)pC->Ee.rData[EeAdd_mbtcpSendAckTimeoutLow] << 0);
	pC->Nic.ncMbWrite.conAckTimeout = ((uint32_t)pC->Ee.rData[EeAdd_mbtcpConnectAckTimeoutHigh] << 16) + ((uint32_t)pC->Ee.rData[EeAdd_mbtcpConnectAckTimeoutLow] << 0);
	pC->Nic.ncMbWrite.closeAckTimeout = ((uint32_t)pC->Ee.rData[EeAdd_mbtcpCloseAckTimeoutHigh] << 16) + ((uint32_t)pC->Ee.rData[EeAdd_mbtcpCloseAckTimeoutLow] << 0);
	pC->Nic.ncMbWrite.ethAddress[0] = pC->Nic.ncMbRead.ethAddress[0];
	pC->Nic.ncMbWrite.ethAddress[1] = pC->Nic.ncMbRead.ethAddress[1];
	pC->Nic.ncMbWrite.ethAddress[2] = pC->Nic.ncMbRead.ethAddress[2];
	pC->Nic.ncMbWrite.ethAddress[3] = pC->Nic.ncMbRead.ethAddress[3];
	pC->Nic.ncMbWrite.ethAddress[4] = pC->Nic.ncMbRead.ethAddress[4];
	pC->Nic.ncMbWrite.ethAddress[5] = pC->Nic.ncMbRead.ethAddress[5];
	

	uint32_t idx = 0;
	NIC_Uint16ToTableUint16(pC->Nic.ncMbWrite.length, &idx, pC->Nic.ncMbWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.busStartup, &idx, pC->Nic.ncMbWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.wdgTimeout, &idx, pC->Nic.ncMbWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.provSerwerConn, &idx, pC->Nic.ncMbWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.responseTimeout/100, &idx, pC->Nic.ncMbWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.clientConWdgTimeout/100, &idx, pC->Nic.ncMbWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.protMode, &idx, pC->Nic.ncMbWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.sendAckTimeout, &idx, pC->Nic.ncMbWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.conAckTimeout, &idx, pC->Nic.ncMbWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.closeAckTimeout, &idx, pC->Nic.ncMbWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.dataSwap, &idx, pC->Nic.ncMbWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.flagsReg321_322, &idx, pC->Nic.ncMbWrite.regs);
	NIC_TableUint8ToTableUint16(pC->Nic.ncMbWrite.ipAddress, pC->Nic.ncMbWrite.regs, &idx, 4);
	NIC_TableUint8ToTableUint16(pC->Nic.ncMbWrite.subnetMask, pC->Nic.ncMbWrite.regs, &idx, 4);
	NIC_TableUint8ToTableUint16(pC->Nic.ncMbWrite.gateway, pC->Nic.ncMbWrite.regs, &idx, 4);
	NIC_TableUint8ToTableUint16(pC->Nic.ncMbWrite.ethAddress, pC->Nic.ncMbWrite.regs, &idx, 6);
	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.flagsReg332_333, &idx, pC->Nic.ncMbWrite.regs);
}
static eResult Control_CompareConfigurationModbusTCP(void)
{
	eResult result = RES_OK;
	for(uint32_t i=0;i<MBTCP_REGMAX;i++)
	{
		if(pC->Nic.ncMbRead.regs[i] != pC->Nic.ncMbWrite.regs[i])
			result = RES_NicNcMbIncompatible;
	}
	return result;
}
static eResult Control_WriteConfigurationModbusTCP(void)
{
	eResult result = RES_OK;
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteNetworkConfigurationMb300_332;
	pC->Nic.mode.tabFunToSend[1] = NIC_WriteClrcfgFlagInCommandFlags;
	pC->Nic.mode.tabFunToSend[2] = NIC_ReadSystemStatusComErrorFlags;
	NIC_StartComunication(2, 10000);
	result = Control_WaitForNicComunicationIsDone(&pC->time1);
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga2 = 1;
	
	result = Control_WaitForNicFlagIsChanged(pC->Nic.sscef.flagFlsCfg, false, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga2 = 2;
	
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteStrcfgFlagInCommandFlags;
	pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemStatusComErrorFlags;
	NIC_StartComunication(2, 10000);
	result = Control_WaitForNicComunicationIsDone(&pC->time3);
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga2 = 3;

	result = Control_WaitForNicFlagIsChanged(pC->Nic.sscef.flagFlsCfg, false, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga2 = 4;
	
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteInitFlagInCommandFlags;
	NIC_StartComunication(1, 10000);
	result = Control_WaitForNicComunicationIsDone(&pC->time5);
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga2 = 5;
	
	return result;
}
static eResult Control_ConfModbusTCP(void)
{
	pC->flaga1 = 0;
	eResult result = RES_OK;
	result = Control_ReadModuleSystemInformation();
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga1 = 1;
	
	result = Control_ComapreSystemInformationModbusTCP();
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga1 = 2;
	
	result = Control_ReadConfigurationFromModuleModbusTCP();
	if(result != RES_OK)
	{
		return result;
	}
	
	Control_PrepareConfigurationToWriteModbusTCP();
	result = Control_CompareConfigurationModbusTCP();
	if(result == RES_OK)
	{
		pC->flaga1 = 3;
		pC->Nic.mode.confStatus = NCS_confIsDone;
		return result;
	}
	pC->flaga1 = 4;
	
	result = Control_WriteConfigurationModbusTCP();
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga1 = 5;
	
	result = Control_ReadConfigurationFromModuleModbusTCP();
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga1 = 6;
	
	Control_PrepareConfigurationToWriteModbusTCP();
	result = Control_CompareConfigurationModbusTCP();
	if(result == RES_OK)
	{
		pC->flaga1 = 7;
		pC->Nic.mode.confStatus = NCS_confIsDone;
		return result;
	}
	else
	{
		pC->flaga1 = 8;
		return result;
	}
}
static void Control_RunModbusTCP(void)
{
	if(pC->Nic.mode.comStatus == NCS_comIsDone)
		pC->Nic.mode.comStatus = NCS_comIsIdle;
	if(pC->Nic.mode.comStatus == NCS_comIsIdle)
	{
		pC->Nic.mode.tabFunToSend[0] = NIC_ReadCoils;
		NIC_StartComunication(1, 50);
	}
}
// ProfiBUS **************************************************************
static eResult Control_ComapreSystemInformationProfiBUS(void)
{
	eResult result = RES_OK;

	if(pC->Nic.si.devNumber != pC->Nic.siDefPfbus.devNumber)
	{
		result = RES_NicSiIncompatible;
		pC->Status.nicIncompDevNumber = true;
	}
	
	if(pC->Nic.si.devClass 	!= pC->Nic.siDefPfbus.devClass)
	{
		result = RES_NicSiIncompatible;
		pC->Status.nicIncompDevClass = true;
	}
	
	if(pC->Nic.si.protClass != pC->Nic.siDefPfbus.protClass)
	{
		result = RES_NicSiIncompatible;
		pC->Status.nicIncompProtClass = true;
	}
	
	for(uint8_t i=0;i<64;i++)
		if(pC->Nic.si.firmName[i] != pC->Nic.siDefPfbus.firmName[i])
		{
			result = RES_NicSiIncompatible;
			pC->Status.nicIncompFirmName = true;
		}
	
	return result;
}
static eResult Control_ReadConfigurationFromModuleProfiBUS(void)
{
	eResult result = RES_OK;
	pC->Nic.mode.tabFunToSend[0] = NIC_ReadNetworkConfigurationPfbus300_399;
	pC->Nic.mode.tabFunToSend[1] = NIC_ReadNetworkConfigurationPfbus400_430;
	NIC_StartComunication(2, 10000);
	result = Control_WaitForNicComunicationIsDone(&pC->time0);
	return result;
}
static void Control_PrepareConfigurationToWriteProfiBUS(void)
{
	pC->Nic.ncPfbusWrite = pC->Nic.ncPfbusDef;
	
	pC->Nic.ncPfbusWrite.wdgTimeout = pC->Ee.wData[EeAdd_pfbusTimeout];
	pC->Nic.ncPfbusWrite.stationAddress = pC->Ee.wData[EeAdd_pfbusAdress];
	pC->Nic.ncPfbusWrite.baudrate = pC->Ee.wData[EeAdd_pfbusBaudrate];
	
	pC->Nic.ncPfbusWrite.flagDpv1Enable = (eBool)pC->Ee.wData[EeAdd_pfbusDPV1Enable]; //register: 307d bit0
	pC->Nic.ncPfbusWrite.flagSyncSupperted = (eBool)pC->Ee.wData[EeAdd_pfbusSyncSupported];
	pC->Nic.ncPfbusWrite.flagFreezeSuported = (eBool)pC->Ee.wData[EeAdd_pfbusFreezeSupported];
	pC->Nic.ncPfbusWrite.flagFailSafeSuported = (eBool)pC->Ee.wData[EeAdd_pfbusFailSafeSupported];
	
	pC->Nic.ncPfbusWrite.flagsReg307 = 0x0000;
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagDpv1Enable << 0);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagSyncSupperted << 1);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagFreezeSuported << 2);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagFailSafeSuported << 3);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagAlarmSap50Deactivate << 4);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagIoDataSwap << 5);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagAutoConfiguration << 6);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagAddressChangeNotAllowed << 7);
	
	pC->Nic.ncPfbusWrite.lengthConfData = 2;
	pC->Nic.ncPfbusWrite.confData[0] = 0xD0E0;
	
	uint32_t idx = 0;
	NIC_Uint16ToTableUint16(pC->Nic.ncPfbusWrite.length, &idx, pC->Nic.ncPfbusWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfbusWrite.flagsReg301_302, &idx, pC->Nic.ncPfbusWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfbusWrite.wdgTimeout, &idx, pC->Nic.ncPfbusWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfbusWrite.identNumber, &idx, pC->Nic.ncPfbusWrite.regs);
	NIC_Uint8ToTableUint16(pC->Nic.ncPfbusWrite.baudrate, pC->Nic.ncPfbusWrite.stationAddress, &idx, pC->Nic.ncPfbusWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfbusWrite.flagsReg307, &idx, pC->Nic.ncPfbusWrite.regs);
	NIC_Uint8ToTableUint16(pC->Nic.ncPfbusWrite.lengthConfData, 0x00, &idx, pC->Nic.ncPfbusWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfbusWrite.confData[0], &idx, pC->Nic.ncPfbusWrite.regs);
}
static eResult Control_CompareConfigurationProfiBUS(void)
{
	eResult result = RES_OK;
	for(uint32_t i=0;i<PFBUS_REGMAX;i++)
	{
		if(pC->Nic.ncPfbusRead.regs[i] != pC->Nic.ncPfbusWrite.regs[i])
			result = RES_NicNcPfbusIncompatible;
	}
	return result;
}
static eResult Control_WriteConfigurationProfiBUS(void)
{
	eResult result = RES_OK;
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteNetworkConfigurationPfbus300_399;
	pC->Nic.mode.tabFunToSend[1] = NIC_WriteNetworkConfigurationPfbus400_430;
	pC->Nic.mode.tabFunToSend[2] = NIC_WriteClrcfgFlagInCommandFlags;
	pC->Nic.mode.tabFunToSend[3] = NIC_ReadSystemStatusComErrorFlags;
	NIC_StartComunication(4, 10000);
	result = Control_WaitForNicComunicationIsDone(&pC->time1);
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga2 = 1;
	
	result = Control_WaitForNicFlagIsChanged(pC->Nic.sscef.flagFlsCfg, false, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga2 = 2;
	
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteStrcfgFlagInCommandFlags;
	pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemStatusComErrorFlags;
	NIC_StartComunication(2, 10000);
	result = Control_WaitForNicComunicationIsDone(&pC->time3);
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga2 = 3;

	result = Control_WaitForNicFlagIsChanged(pC->Nic.sscef.flagFlsCfg, false, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga2 = 4;
	
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteInitFlagInCommandFlags;
	NIC_StartComunication(1, 10000);
	result = Control_WaitForNicComunicationIsDone(&pC->time5);
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga2 = 5;
	
	return result;
}
static eResult Control_ConfProfiBUS(void)
{
	pC->flaga1 = 0;
	eResult result = RES_OK;
	result = Control_ReadModuleSystemInformation();
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga1 = 1;
	
	result = Control_ComapreSystemInformationProfiBUS();
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga1 = 2;
	
	result = Control_ReadConfigurationFromModuleProfiBUS();
	if(result != RES_OK)
	{
		return result;
	}
	
	Control_PrepareConfigurationToWriteProfiBUS();
	result = Control_CompareConfigurationProfiBUS();
	if(result == RES_OK)
	{
		pC->flaga1 = 3;
		pC->Nic.mode.confStatus = NCS_confIsDone;
		return result;
	}
	pC->flaga1 = 4;
	
	result = Control_WriteConfigurationProfiBUS();
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga1 = 5;
	
	result = Control_ReadConfigurationFromModuleProfiBUS();
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga1 = 6;
	
	Control_PrepareConfigurationToWriteProfiBUS();
	result = Control_CompareConfigurationProfiBUS();
	if(result == RES_OK)
	{
		pC->flaga1 = 7;
		pC->Nic.mode.confStatus = NCS_confIsDone;
		return result;
	}
	else
	{
		pC->flaga1 = 8;
		return result;
	}
}
static void Control_RunProfiBUS(void)
{
	if(pC->Nic.mode.comStatus == NCS_comIsDone)
		pC->Nic.mode.comStatus = NCS_comIsIdle;
	if(pC->Nic.mode.comStatus == NCS_comIsIdle)
	{
		pC->Nic.mode.tabFunToSend[0] = NIC_ReadCoils;
		NIC_StartComunication(1, 50);
	}
}
// ProfiNET ************************************************************
static eResult Control_ComapreSystemInformationProfiNET(void)
{
	eResult result = RES_OK;

	if(pC->Nic.si.devNumber != pC->Nic.siDefPfnet.devNumber)
	{
		result = RES_NicSiIncompatible;
		pC->Status.nicIncompDevNumber = true;
	}
	
	if(pC->Nic.si.devClass 	!= pC->Nic.siDefPfnet.devClass)
	{
		result = RES_NicSiIncompatible;
		pC->Status.nicIncompDevClass = true;
	}
	
	if(pC->Nic.si.protClass != pC->Nic.siDefPfnet.protClass)
	{
		result = RES_NicSiIncompatible;
		pC->Status.nicIncompProtClass = true;
	}
	
	for(uint8_t i=0;i<64;i++)
		if(pC->Nic.si.firmName[i] != pC->Nic.siDefPfnet.firmName[i])
		{
			result = RES_NicSiIncompatible;
			pC->Status.nicIncompFirmName = true;
		}
	
	return result;
}
static eResult Control_ReadConfigurationFromModuleProfiNET(void)
{
	eResult result = RES_OK;
	pC->Nic.mode.tabFunToSend[0] = NIC_ReadNetworkConfigurationPfnet300_399;
	pC->Nic.mode.tabFunToSend[1] = NIC_ReadNetworkConfigurationPfnet400_499;
	pC->Nic.mode.tabFunToSend[2] = NIC_ReadNetworkConfigurationPfnet500_598;
	pC->Nic.mode.tabFunToSend[3] = NIC_ReadNetworkConfigurationPfnet599_699;
	pC->Nic.mode.tabFunToSend[4] = NIC_ReadNetworkConfigurationPfnet700_799;
	pC->Nic.mode.tabFunToSend[5] = NIC_ReadNetworkConfigurationPfnet800_899;
	pC->Nic.mode.tabFunToSend[6] = NIC_ReadNetworkConfigurationPfnet900_987;
	NIC_StartComunication(7, 10000);
	result = Control_WaitForNicComunicationIsDone(&pC->time0);
	return result;
}
static void Control_PrepareConfigurationToWriteProfiNET(void)
{
//	pC->Nic.ncMbWrite = pC->Nic.ncMbDef;
//	
//	pC->Nic.ncMbWrite.wdgTimeout = pC->Ee.rData[EeAdd_mbtcpTimeout];
//	pC->Nic.ncMbWrite.dataSwap = pC->Ee.rData[EeAdd_mbtcpDataSwap];
//	pC->Nic.ncMbWrite.ipAddress[0] = pC->Ee.rData[EeAdd_mbtcpIP0];
//	pC->Nic.ncMbWrite.ipAddress[1] = pC->Ee.rData[EeAdd_mbtcpIP1];
//	pC->Nic.ncMbWrite.ipAddress[2] = pC->Ee.rData[EeAdd_mbtcpIP2];
//	pC->Nic.ncMbWrite.ipAddress[3] = pC->Ee.rData[EeAdd_mbtcpIP3];
//	pC->Nic.ncMbWrite.subnetMask[0] = pC->Ee.rData[EeAdd_mbtcpMask0];
//	pC->Nic.ncMbWrite.subnetMask[1] = pC->Ee.rData[EeAdd_mbtcpMask1];
//	pC->Nic.ncMbWrite.subnetMask[2] = pC->Ee.rData[EeAdd_mbtcpMask2];
//	pC->Nic.ncMbWrite.subnetMask[3] = pC->Ee.rData[EeAdd_mbtcpMask3];
//	pC->Nic.ncMbWrite.gateway[0] = pC->Ee.rData[EeAdd_mbtcpGateway0];
//	pC->Nic.ncMbWrite.gateway[1] = pC->Ee.rData[EeAdd_mbtcpGateway1];
//	pC->Nic.ncMbWrite.gateway[2] = pC->Ee.rData[EeAdd_mbtcpGateway2];
//	pC->Nic.ncMbWrite.gateway[3] = pC->Ee.rData[EeAdd_mbtcpGateway3];
//	pC->Nic.ncMbWrite.provSerwerConn = pC->Ee.rData[EeAdd_mbtcpSerwerCons];
//	pC->Nic.ncMbWrite.sendAckTimeout = ((uint32_t)pC->Ee.rData[EeAdd_mbtcpSendAckTimeoutHigh] << 16) + ((uint32_t)pC->Ee.rData[EeAdd_mbtcpSendAckTimeoutLow] << 0);
//	pC->Nic.ncMbWrite.conAckTimeout = ((uint32_t)pC->Ee.rData[EeAdd_mbtcpConnectAckTimeoutHigh] << 16) + ((uint32_t)pC->Ee.rData[EeAdd_mbtcpConnectAckTimeoutLow] << 0);
//	pC->Nic.ncMbWrite.closeAckTimeout = ((uint32_t)pC->Ee.rData[EeAdd_mbtcpCloseAckTimeoutHigh] << 16) + ((uint32_t)pC->Ee.rData[EeAdd_mbtcpCloseAckTimeoutLow] << 0);
//	pC->Nic.ncMbWrite.ethAddress[0] = pC->Nic.ncMbRead.ethAddress[0];
//	pC->Nic.ncMbWrite.ethAddress[1] = pC->Nic.ncMbRead.ethAddress[1];
//	pC->Nic.ncMbWrite.ethAddress[2] = pC->Nic.ncMbRead.ethAddress[2];
//	pC->Nic.ncMbWrite.ethAddress[3] = pC->Nic.ncMbRead.ethAddress[3];
//	pC->Nic.ncMbWrite.ethAddress[4] = pC->Nic.ncMbRead.ethAddress[4];
//	pC->Nic.ncMbWrite.ethAddress[5] = pC->Nic.ncMbRead.ethAddress[5];
//	

//	uint32_t idx = 0;
//	NIC_Uint16ToTableUint16(pC->Nic.ncMbWrite.length, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.busStartup, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.wdgTimeout, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.provSerwerConn, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.responseTimeout/100, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.clientConWdgTimeout/100, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.protMode, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.sendAckTimeout, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.conAckTimeout, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.closeAckTimeout, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.dataSwap, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.flagsReg321_322, &idx, pC->Nic.ncMbWrite.regs);
//	NIC_TableUint8ToTableUint16(pC->Nic.ncMbWrite.ipAddress, pC->Nic.ncMbWrite.regs, &idx, 4);
//	NIC_TableUint8ToTableUint16(pC->Nic.ncMbWrite.subnetMask, pC->Nic.ncMbWrite.regs, &idx, 4);
//	NIC_TableUint8ToTableUint16(pC->Nic.ncMbWrite.gateway, pC->Nic.ncMbWrite.regs, &idx, 4);
//	NIC_TableUint8ToTableUint16(pC->Nic.ncMbWrite.ethAddress, pC->Nic.ncMbWrite.regs, &idx, 6);
//	NIC_Uint32ToTableUint16(pC->Nic.ncMbWrite.flagsReg332_333, &idx, pC->Nic.ncMbWrite.regs);
}
static eResult Control_CompareConfigurationProfiNET(void)
{
	eResult result = RES_OK;
//	for(uint32_t i=0;i<100;i++)
//	{
//		if(pC->Nic.ncMbRead.regs[i] != pC->Nic.ncMbWrite.regs[i])
//			result = RES_NicNcMbIncompatible;
//	}
	return result;
}
static eResult Control_WriteConfigurationProfiNET(void)
{
	eResult result = RES_OK;
//	pC->Nic.mode.tabFunToSend[0] = NIC_WriteNetworkConfigurationMb;
//	pC->Nic.mode.tabFunToSend[1] = NIC_WriteClrcfgFlagInCommandFlags;
//	pC->Nic.mode.tabFunToSend[2] = NIC_ReadSystemStatusComErrorFlags;
//	NIC_StartComunication(2, 10000);
//	result = Control_WaitForNicComunicationIsDone(&pC->time1);
//	if(result != RES_OK)
//	{
//		return result;
//	}
//	pC->flaga2 = 1;
//	
//	result = Control_WaitForNicFlagIsChanged(pC->Nic.sscef.flagFlsCfg, false, 10000);
//	if(result != RES_OK)
//	{
//		return result;
//	}
//	pC->flaga2 = 2;
//	
//	pC->Nic.mode.tabFunToSend[0] = NIC_WriteStrcfgFlagInCommandFlags;
//	pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemStatusComErrorFlags;
//	NIC_StartComunication(2, 10000);
//	result = Control_WaitForNicComunicationIsDone(&pC->time3);
//	if(result != RES_OK)
//	{
//		return result;
//	}
//	pC->flaga2 = 3;

//	result = Control_WaitForNicFlagIsChanged(pC->Nic.sscef.flagFlsCfg, false, 10000);
//	if(result != RES_OK)
//	{
//		return result;
//	}
//	pC->flaga2 = 4;
//	
//	pC->Nic.mode.tabFunToSend[0] = NIC_WriteInitFlagInCommandFlags;
//	NIC_StartComunication(1, 10000);
//	result = Control_WaitForNicComunicationIsDone(&pC->time5);
//	if(result != RES_OK)
//	{
//		return result;
//	}
//	pC->flaga2 = 5;
	
	return result;
}
static eResult Control_ConfProfiNET(void)
{
	pC->flaga1 = 0;
	eResult result = RES_OK;
	result = Control_ReadModuleSystemInformation();
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga1 = 1;
	
	result = Control_ComapreSystemInformationProfiNET();
	if(result != RES_OK)
	{
		return result;
	}
	pC->flaga1 = 2;
	
	result = Control_ReadConfigurationFromModuleProfiNET();
	if(result != RES_OK)
	{
		return result;
	}
	
//	Control_PrepareConfigurationToWriteProfiNET();
//	result = Control_CompareConfigurationProfiNET();
//	if(result == RES_OK)
//	{
//		pC->flaga1 = 3;
//		pC->Nic.mode.confStatus = NCS_confIsDone;
//		return result;
//	}
//	pC->flaga1 = 4;
//	
//	result = Control_WriteConfigurationProfiNET();
//	if(result != RES_OK)
//	{
//		return result;
//	}
//	pC->flaga1 = 5;
//	
//	result = Control_ReadConfigurationFromModuleProfiNET();
//	if(result != RES_OK)
//	{
//		return result;
//	}
//	pC->flaga1 = 6;
//	
//	Control_PrepareConfigurationToWriteProfiNET();
//	result = Control_CompareConfigurationProfiNET();
//	if(result == RES_OK)
//	{
//		pC->flaga1 = 7;
//		pC->Nic.mode.confStatus = NCS_confIsDone;
//		return result;
//	}
//	else
//	{
//		pC->flaga1 = 8;
//		return result;
//	}
}
static void Control_RunProfiNET(void)
{
	if(pC->Nic.mode.comStatus == NCS_comIsDone)
		pC->Nic.mode.comStatus = NCS_comIsIdle;
	if(pC->Nic.mode.comStatus == NCS_comIsIdle)
	{
		pC->Nic.mode.tabFunToSend[0] = NIC_ReadCoils;
		NIC_StartComunication(1, 50);
	}
}
//*******************************************************************************
static void Control_WorkTypeStop(void)
{
	Outputs_WorkTypeStop();
}
void Control_WorkTypeConf(void)
{
	delay_ms(3000);
	eResult result = RES_OK;
	pC->Mode.workType = workTypeConf;
	Outputs_WorkTypeConf();
	
	for(uint8_t i=0;i<3;i++)
	{
		if(pC->Mode.protocol == Prot_Mbrtu)
			result = Control_ConfModbusRTU();
		else if(pC->Mode.protocol == Prot_Mbtcp)
			result = Control_ConfModbusTCP();
		else if(pC->Mode.protocol == Prot_Pfbus)
			result = Control_ConfProfiBUS();
		else if(pC->Mode.protocol == Prot_Pfnet)
			Control_ConfProfiNET();
		
		pC->Status.nicComTimeoutError = false;
		if(result == RES_OK)
			break;
	}
	
	if(result == RES_OK)
	{
		pC->Mode.workType = workTypeRun;
	}
	else
	{
		pC->Mode.workType = workTypeError;
	}
}
static void Control_WorkTypeRun(void)
{
	pC->Mode.ledPeriod = 500;
	Outputs_WorkTypeRun();
	if(pC->Mode.protocol == Prot_Mbrtu)
	{
		Control_RunModbusRTU();
	}
	else if(pC->Mode.protocol == Prot_Mbtcp)
	{
		Control_RunModbusTCP();
	}
	else if(pC->Mode.protocol == Prot_Pfbus)
	{
		Control_RunProfiBUS();
	}
	else if(pC->Mode.protocol == Prot_Pfnet)
	{
		Control_RunProfiNET();
	}
}
static void Control_WorkTypeError(void)
{
	pC->Mode.ledPeriod = 100;
	Outputs_WorkTypeError();
}
//*******************************************************************************
static void Control_Act(void)
{
	Control_LedAct();
	Control_ReadMcuTemp();
	if(pC->Mode.workType == workTypeRun)
	{
		Control_WorkTypeRun();
	}
	else if(pC->Mode.workType == workTypeError)
	{
		Control_WorkTypeError();
	}
}
//***** Interrupts ********************************
void TIM6_DAC_IRQHandler(void)
{
	if((TIM6->SR & TIM_SR_UIF) != RESET)
	{
		Control_Act();
		TIM6->SR &= ~TIM_SR_UIF;
	}
}
