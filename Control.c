#include "Control.h"
uint32_t flaga1 = 0;
extern sControl* pC;
static eResult Control_RccSystemInit(void)
{
	eResult result = RES_OK;
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
	return result;
}
static eResult Control_RccConf(void)
{
	eResult result = RES_OK;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_TIM6EN;
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_ADC1EN;
	
	return result;
}
static eResult Control_LedConf(void)
{
	eResult result = RES_OK;
	LED_PORT->MODER 	|= GPIO_MODER_MODER8_0;
	LED_PORT->PUPDR 	|= GPIO_PUPDR_PUPDR8_0;
	LED1_OFF;
	
	return result;
}
static eResult Control_StructConf(void)
{
	eResult result = RES_OK;
	pC->Mode.protocol = Prot_Mbrtu;
	pC->Mode.workType = workTypeStop;
	pC->Mode.dataSwapMbrtu = true;
	pC->Mode.dataSwapMbtcp = true;
	pC->Mode.dataSwapPfbus = true;
	pC->Mode.dataSwapPfnet = true;
	pC->Mode.ledTime = 0;
	pC->Mode.ledPeriod = 0;
	
	for(uint32_t i=0;i<EE_VARMAX;i++)
		pC->Ee.VirtAddVarTab[i] = i;
	
	pC->Mbs.address = 2;
	pC->Mbs.baud = 9600;
	pC->Mbs.parity = 0;
	
	pC->Nic.mode.nicFun = NF_I;
	pC->Nic.mode.confStatus = NCS_confIsntDone;
	pC->Nic.mode.comStatus = NCS_comIsIdle;
	pC->Nic.mode.comBaud = 115200;
	pC->Nic.mode.comAddress = 2;
	pC->Nic.mode.comTime = 0;
	pC->Nic.mode.comTimeout = 100;
	pC->Nic.mode.flagTime = 0;
	pC->Nic.mode.flagTimeout = 1000;
	pC->Nic.mode.numFunToSend = 0;
	pC->Nic.mode.maxFunToSend = 0;
	pC->Nic.mode.comPossibleSettings[0] = 1200;
	pC->Nic.mode.comPossibleSettings[1] = 2400;
	pC->Nic.mode.comPossibleSettings[2] = 4800;
	pC->Nic.mode.comPossibleSettings[3] = 9600;
	pC->Nic.mode.comPossibleSettings[4] = 19200;
	pC->Nic.mode.comPossibleSettings[5] = 38400;
	pC->Nic.mode.comPossibleSettings[6] = 57600;
	pC->Nic.mode.comPossibleSettings[7] = 115200;
	
	NIC_SetDefaultSystemConfiguration();
	NIC_SetDefaultSystemInformationMb();
	NIC_SetDefaultConfigurationMb();
	NIC_SetDefaultSystemInformationPfbus();
	NIC_SetDefaultConfigurationPfbus();
	NIC_SetDefaultSystemInformationPfnet();
	NIC_SetDefaultConfigurationPfnet();
	
	return result;
}
static eResult Control_AdcConf(void)
{
	eResult result = RES_OK;
	DMA2_Stream0->PAR = (uint32_t)&ADC1->DR;
	DMA2_Stream0->M0AR = (uint32_t)pC->Status.statAdcValue;
	DMA2_Stream0->NDTR = (uint16_t)(200);
	DMA2_Stream0->CR |= DMA_SxCR_PSIZE_0 | DMA_SxCR_MSIZE_0 | DMA_SxCR_MINC | DMA_SxCR_CIRC | DMA_SxCR_EN;
	
	ADC->CCR 	|= ADC_CCR_ADCPRE | ADC_CCR_TSVREFE;
	ADC1->CR2 |= ADC_CR2_ADON | ADC_CR2_CONT | ADC_CR2_DDS | ADC_CR2_DMA;
	ADC1->CR1 |= ADC_CR1_SCAN;
	
	ADC1->SMPR1 |= ADC_SMPR1_SMP18;
	ADC1->SQR3 |= (18<<0);
	ADC1->CR2 |= ADC_CR2_SWSTART;
	
	return result;
}
static eResult Control_ReadConfigFromFlash(void)
{
	eResult result = RES_OK;
	FLASH_Unlock();
	EE_Init();
	uint16_t check = 0x0000;
	EE_ReadVariable(pC->Ee.VirtAddVarTab[0], &check);
	if(check == EE_CONFIGWASUPLOADED)
	{
		for(uint16_t i=0;i<EE_VARMAX;i++)
			EE_ReadVariable(pC->Ee.VirtAddVarTab[i], &pC->Ee.rData[i]);
		pC->Mode.protocol = (eProtocol)pC->Ee.rData[EeAdd_stmProt];
		pC->Mode.dataSwapMbrtu = (eBool)pC->Ee.rData[EeAdd_mbrtuDataSwap];
		pC->Mode.dataSwapMbtcp = (eBool)pC->Ee.rData[EeAdd_mbtcpDataSwap];
		pC->Mode.dataSwapPfbus = (eBool)pC->Ee.rData[EeAdd_pfbusDataSwap];
		pC->Mode.dataSwapPfnet = (eBool)pC->Ee.rData[EeAdd_pfnetDataSwap];
	}
	else
	{
		result = RES_EeReadingConfigIncorrect;
	}
	FLASH_Lock();
	
	return result;
}
eResult Control_WriteConfigToFlash(void)
{
	eResult result = RES_OK;
	pC->Ee.wData[EeAdd_configWasUploaded] 					= (uint16_t)EE_CONFIGWASUPLOADED;
	pC->Ee.wData[EeAdd_stmProt] 										= (uint16_t)Prot_Mbrtu;
	
	pC->Ee.wData[EeAdd_mbrtuTimeout] 								= 1000;
	pC->Ee.wData[EeAdd_mbrtuDataSwap] 							= 0;
	pC->Ee.wData[EeAdd_mbrtuAddress] 								= 5;
	pC->Ee.wData[EeAdd_mbrtuBaudrate] 							= 3;
	pC->Ee.wData[EeAdd_mbrtuParity] 								= 0;
	
	pC->Ee.wData[EeAdd_mbtcpTimeout] 								= 1001;
	pC->Ee.wData[EeAdd_mbtcpDataSwap] 							= 1;
	pC->Ee.wData[EeAdd_mbtcpIP0] 										= 19;
	pC->Ee.wData[EeAdd_mbtcpIP1] 										= 0;
	pC->Ee.wData[EeAdd_mbtcpIP2] 										= 168;
	pC->Ee.wData[EeAdd_mbtcpIP3] 										= 192;
	pC->Ee.wData[EeAdd_mbtcpMask0] 									= 0;
	pC->Ee.wData[EeAdd_mbtcpMask1] 									= 255;
	pC->Ee.wData[EeAdd_mbtcpMask2] 									= 255;
	pC->Ee.wData[EeAdd_mbtcpMask3] 									= 255;
	pC->Ee.wData[EeAdd_mbtcpGateway0] 							= 1;
	pC->Ee.wData[EeAdd_mbtcpGateway1] 							= 0;
	pC->Ee.wData[EeAdd_mbtcpGateway2] 							= 168;
	pC->Ee.wData[EeAdd_mbtcpGateway3] 							= 192;
	pC->Ee.wData[EeAdd_mbtcpSerwerCons] 						= 16;
	pC->Ee.wData[EeAdd_mbtcpSendAckTimeoutLow] 			= (uint16_t)31000;
	pC->Ee.wData[EeAdd_mbtcpSendAckTimeoutHigh] 		= (uint16_t)(31000 >> 16);
	pC->Ee.wData[EeAdd_mbtcpConnectAckTimeoutLow] 	= (uint16_t)31000;
	pC->Ee.wData[EeAdd_mbtcpConnectAckTimeoutHigh] 	= (uint16_t)(31000 >> 16);
	pC->Ee.wData[EeAdd_mbtcpCloseAckTimeoutLow] 		= (uint16_t)13000;
	pC->Ee.wData[EeAdd_mbtcpCloseAckTimeoutHigh] 		= (uint16_t)(13000 >> 16);
	
	pC->Ee.wData[EeAdd_pfbusTimeout] 								= 1001;
	pC->Ee.wData[EeAdd_pfbusDataSwap] 							= 1;
	pC->Ee.wData[EeAdd_pfbusIdentNumber] 						= 0x0C10;
	pC->Ee.wData[EeAdd_pfbusAdress] 								= 3;
	pC->Ee.wData[EeAdd_pfbusBaudrate] 							= 15;
	pC->Ee.wData[EeAdd_pfbusDPV1Enable] 						= 1;
	pC->Ee.wData[EeAdd_pfbusSyncSupported] 					= 1;
	pC->Ee.wData[EeAdd_pfbusFreezeSupported] 				= 1;
	pC->Ee.wData[EeAdd_pfbusFailSafeSupported] 			= 1;
	
	pC->Ee.wData[EeAdd_pfnetTimeout] 							= 1001;
	pC->Ee.wData[EeAdd_pfnetDataSwap] 						= 1;
	pC->Ee.wData[EeAdd_pfnetVendorId] 						= 0x011E;
	pC->Ee.wData[EeAdd_pfnetDeviceId] 						= 0x010A;
	pC->Ee.wData[EeAdd_pfnetLengthNameOfStation] 	= 10;
	pC->Ee.wData[EeAdd_pfnetNameOfStation + 0] 			= 'n';
	pC->Ee.wData[EeAdd_pfnetNameOfStation + 1] 			= 'i';
	pC->Ee.wData[EeAdd_pfnetNameOfStation + 2] 			= 'c';
	pC->Ee.wData[EeAdd_pfnetNameOfStation + 3] 			= '5';
	pC->Ee.wData[EeAdd_pfnetNameOfStation + 4] 			= '0';
	pC->Ee.wData[EeAdd_pfnetNameOfStation + 5] 			= 'r';
	pC->Ee.wData[EeAdd_pfnetNameOfStation + 6] 			= 'e';
	pC->Ee.wData[EeAdd_pfnetNameOfStation + 7] 			= 'p';
	pC->Ee.wData[EeAdd_pfnetNameOfStation + 8] 			= 'n';
	pC->Ee.wData[EeAdd_pfnetNameOfStation + 9] 			= 's';
	for(uint16_t i=pC->Ee.wData[EeAdd_pfnetLengthNameOfStation];i<240;i++)
		pC->Ee.wData[EeAdd_pfnetNameOfStation + i] 		= 0;
	pC->Ee.wData[EeAdd_pfnetLengthTypeOfStation] 		= 19;
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 0] 			= 'D';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 1] 			= 'e';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 2] 			= 'f';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 3] 			= 'a';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 4] 			= 'u';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 5] 			= 'l';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 6] 			= 't';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 7] 			= '.';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 8] 			= 'D';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 9] 			= 'e';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 10] 		= 'v';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 11] 		= 'i';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 12] 		= 'c';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 13] 		= 'e';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 14] 		= '.';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 15] 		= 'T';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 16] 		= 'y';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 17] 		= 'p';
	pC->Ee.wData[EeAdd_pfnetTypeOfStation + 18] 		= 'e';
	for(uint16_t i=pC->Ee.wData[EeAdd_pfnetLengthTypeOfStation];i<240;i++)
		pC->Ee.wData[EeAdd_pfnetTypeOfStation + i] 		= 0;
	pC->Ee.wData[EeAdd_pfnetDeviceType + 0] 				= 'D';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 1] 				= 'e';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 2] 				= 'f';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 3] 				= 'a';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 4] 				= 'u';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 5] 				= 'l';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 6] 				= 't';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 7] 				= '.';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 8] 				= 'D';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 9] 				= 'e';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 10] 				= 'v';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 11] 				= 'i';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 12] 				= 'c';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 13] 				= 'e';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 14] 				= '.';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 15] 				= 'T';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 16] 				= 'y';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 17] 				= 'p';
	pC->Ee.wData[EeAdd_pfnetDeviceType + 18] 				= 'e';
	for(uint16_t i=19;i<28;i++)
		pC->Ee.wData[EeAdd_pfnetDeviceType + i] 			= 0;
	pC->Ee.wData[EeAdd_pfnetOrderId + 0] 			= '1';
	pC->Ee.wData[EeAdd_pfnetOrderId + 1] 			= '5';
	pC->Ee.wData[EeAdd_pfnetOrderId + 2] 			= '4';
	pC->Ee.wData[EeAdd_pfnetOrderId + 3] 			= '1';
	pC->Ee.wData[EeAdd_pfnetOrderId + 4] 			= '.';
	pC->Ee.wData[EeAdd_pfnetOrderId + 5] 			= 'l';
	pC->Ee.wData[EeAdd_pfnetOrderId + 6] 			= '0';
	pC->Ee.wData[EeAdd_pfnetOrderId + 7] 			= '0';
	for(uint16_t i=7;i<20;i++)
		pC->Ee.wData[EeAdd_pfnetOrderId + i] 		= 0;
	pC->Ee.wData[EeAdd_pfnetIP0] 										= 20;
	pC->Ee.wData[EeAdd_pfnetIP1] 										= 0;
	pC->Ee.wData[EeAdd_pfnetIP2] 										= 168;
	pC->Ee.wData[EeAdd_pfnetIP3] 										= 192;
	pC->Ee.wData[EeAdd_pfnetMask0] 									= 0;
	pC->Ee.wData[EeAdd_pfnetMask1] 									= 255;
	pC->Ee.wData[EeAdd_pfnetMask2] 									= 255;
	pC->Ee.wData[EeAdd_pfnetMask3] 									= 255;
	pC->Ee.wData[EeAdd_pfnetGateway0] 							= 1;
	pC->Ee.wData[EeAdd_pfnetGateway1] 							= 0;
	pC->Ee.wData[EeAdd_pfnetGateway2] 							= 168;
	pC->Ee.wData[EeAdd_pfnetGateway3] 							= 192;
	
	pC->Ee.wData[EeAdd_pfnetHardwareRevision] 			= 1;
	pC->Ee.wData[EeAdd_pfnetSoftwareRevision1] 			= 1;
	pC->Ee.wData[EeAdd_pfnetSoftwareRevision2] 			= 5;
	pC->Ee.wData[EeAdd_pfnetSoftwareRevision3] 			= 0;
	pC->Ee.wData[EeAdd_pfnetSoftwareRevisionPrefix] = ((uint16_t)'R' << 0);
	
	FLASH_Unlock();
	EE_Init();
	for(uint16_t i=0;i<EE_VARMAX;i++)
		EE_WriteVariable(pC->Ee.VirtAddVarTab[i], pC->Ee.wData[i]);
	FLASH_Lock();
	
	return result;
}

static eResult Control_TimsConf(void)
{
	eResult result = RES_OK;
	TIM6->PSC = 50-1;
	TIM6->ARR = 10000;
	TIM6->DIER |= TIM_DIER_UIE;
	TIM6->CR1 |= TIM_CR1_CEN;
	NVIC_EnableIRQ(TIM6_DAC_IRQn);
	return result;
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
		LED1_OFF;
		pC->Mode.ledTime = 0;
	}
	else if(pC->Mode.workType == workTypeRun)
	{
		LED1_ON;
		pC->Mode.ledTime = 0;
	}
	else if(pC->Mode.workType == workTypeError)
	{
		pC->Mode.ledPeriod = 100;
		if(pC->Mode.ledTime >= pC->Mode.ledPeriod)
		{
			LED1_TOG;
			pC->Mode.ledTime = 0;
		}
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
	if(pC->Nic.mode.flagTime >= pC->Nic.mode.flagTimeout)
		pC->Status.flagNicFlagTimeoutError = true;
	else
		pC->Status.flagNicFlagTimeoutError = false;
	
	if(pC->Nic.mode.comStatus != NCS_comIsIdle && pC->Nic.mode.comStatus != NCS_comIsDone)
		pC->Nic.mode.comTime++;
	if(pC->Nic.mode.comTime >= pC->Nic.mode.comTimeout)
		pC->Status.flagNicComTimeoutError = true;
	else
		pC->Status.flagNicComTimeoutError = false;
	
	
	if(pC->Mode.protocol == Prot_Mbrtu)
	{
//		pC->Status.statBusMbrtuSystemError = pC->Nic.sscef.systemError;
//		pC->Status.statBusMbrtuCounterError = pC->Nic.sscef.errorCounter;
//		pC->Status.statBusMbrtuComError = pC->Nic.sscef.comError;
//		pC->Status.statBusMbrtuComStatus = pC->Nic.sscef.comStatus;
//		pC->Status.flagBusMbrtuReady = pC->Nic.sscef.flagReady;
//		pC->Status.flagBusMbrtuError = pC->Nic.sscef.flagError;
//		pC->Status.flagBusMbrtuCommunicating = pC->Nic.sscef.flagCommunicating;
//		pC->Status.flagBusMbrtuNcfError = pC->Nic.sscef.flagNcfError;
//		pC->Status.flagBusMbrtuBusOn = pC->Nic.sscef.flagBusOn;
//		pC->Status.flagBusMbrtuRunning = pC->Nic.sscef.flagRunning;
	}
	if(pC->Status.flagBusMbtcpCorrectModule == true)
	{
		pC->Status.statBusMbtcpSystemError = pC->Nic.sscef.systemError;
		pC->Status.statBusMbtcpCounterError = pC->Nic.sscef.errorCounter;
		pC->Status.statBusMbtcpComError = pC->Nic.sscef.comError;
		pC->Status.statBusMbtcpComStatus = pC->Nic.sscef.comStatus;
		pC->Status.flagBusMbtcpReady = pC->Nic.sscef.flagReady;
		pC->Status.flagBusMbtcpError = pC->Nic.sscef.flagError;
		pC->Status.flagBusMbtcpCommunicating = pC->Nic.sscef.flagCommunicating;
		pC->Status.flagBusMbtcpNcfError = pC->Nic.sscef.flagNcfError;
		pC->Status.flagBusMbtcpBusOn = pC->Nic.sscef.flagBusOn;
		pC->Status.flagBusMbtcpRunning = pC->Nic.sscef.flagRunning;
	}
	if(pC->Status.flagBusPfbusCorrectModule == true)
	{
		pC->Status.statBusPfbusSystemError = pC->Nic.sscef.systemError;
		pC->Status.statBusPfbusCounterError = pC->Nic.sscef.errorCounter;
		pC->Status.statBusPfbusComError = pC->Nic.sscef.comError;
		pC->Status.statBusPfbusComStatus = pC->Nic.sscef.comStatus;
		pC->Status.flagBusPfbusReady = pC->Nic.sscef.flagReady;
		pC->Status.flagBusPfbusError = pC->Nic.sscef.flagError;
		pC->Status.flagBusPfbusCommunicating = pC->Nic.sscef.flagCommunicating;
		pC->Status.flagBusPfbusNcfError = pC->Nic.sscef.flagNcfError;
		pC->Status.flagBusPfbusBusOn = pC->Nic.sscef.flagBusOn;
		pC->Status.flagBusPfbusRunning = pC->Nic.sscef.flagRunning;
	}
	if(pC->Status.flagBusPfnetCorrectModule == true)
	{
		pC->Status.statBusPfnetSystemError = pC->Nic.sscef.systemError;
		pC->Status.statBusPfnetCounterError = pC->Nic.sscef.errorCounter;
		pC->Status.statBusPfnetComError = pC->Nic.sscef.comError;
		pC->Status.statBusPfnetComStatus = pC->Nic.sscef.comStatus;
		pC->Status.flagBusPfnetReady = pC->Nic.sscef.flagReady;
		pC->Status.flagBusPfnetError = pC->Nic.sscef.flagError;
		pC->Status.flagBusPfnetCommunicating = pC->Nic.sscef.flagCommunicating;
		pC->Status.flagBusPfnetNcfError = pC->Nic.sscef.flagNcfError;
		pC->Status.flagBusPfnetBusOn = pC->Nic.sscef.flagBusOn;
		pC->Status.flagBusPfnetRunning = pC->Nic.sscef.flagRunning;
	}
	
	pC->Status.flagsMbrtu = 0x00000000;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuCorrectModule << 0;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuCorrectDevNumber << 1;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuCorrectDevClass << 2;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuCorrectProtClass << 3;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuCorrectFirmName << 4;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuConfigUploadError << 5;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuConfigUploaded << 6;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuReady << 7;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuError << 8;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuCommunicating << 9;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuNcfError << 10;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuBusOn << 11;
	pC->Status.flagsMbrtu += pC->Status.flagBusMbrtuRunning << 12;
	
	pC->Status.flagsMbtcp = 0x00000000;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpCorrectModule << 0;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpCorrectDevNumber << 1;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpCorrectDevClass << 2;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpCorrectProtClass << 3;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpCorrectFirmName << 4;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpConfigUploadError << 5;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpConfigUploaded << 6;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpReady << 7;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpError << 8;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpCommunicating << 9;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpNcfError << 10;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpBusOn << 11;
	pC->Status.flagsMbtcp += pC->Status.flagBusMbtcpRunning << 12;
	
	pC->Status.flagsPfbus = 0x00000000;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusCorrectModule << 0;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusCorrectDevNumber << 1;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusCorrectDevClass << 2;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusCorrectProtClass << 3;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusCorrectFirmName << 4;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusConfigUploadError << 5;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusConfigUploaded << 6;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusReady << 7;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusError << 8;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusCommunicating << 9;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusNcfError << 10;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusBusOn << 11;
	pC->Status.flagsPfbus += pC->Status.flagBusPfbusRunning << 12;
	
	pC->Status.flagsPfnet = 0x00000000;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetCorrectModule << 0;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetCorrectDevNumber << 1;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetCorrectDevClass << 2;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetCorrectProtClass << 3;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetCorrectFirmName << 4;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetConfigUploadError << 5;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetConfigUploaded << 6;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetReady << 7;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetError << 8;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetCommunicating << 9;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetNcfError << 10;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetBusOn << 11;
	pC->Status.flagsPfnet += pC->Status.flagBusPfnetRunning << 12;
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
		suma += pC->Status.statAdcValue[i];
	
	double VSENSE = (double)suma / 200.0 / 4096.0 * 3.3;
	double V25 = 0.76;//V
	double Avg_Slope = 0.0025; //V/C
	pC->Status.statTemp = ((VSENSE - V25) / Avg_Slope) + 25.0;
}
//*******************************************************************************
uint16_t Control_DataSwap(uint16_t in, eBool flag)
{
	uint16_t out = 0x0000;
	uint16_t temp = 0x0000;
	if(flag == true)
	{
		temp = in >> 8;
		out = (in << 8) + temp;
	}
	else
	{
		out = in;
	}
		
	return out;
}
static eResult Control_WaitForNicComunicationIsDone(void)
{
	eResult result = RES_OK;
	while(pC->Nic.mode.comStatus != NCS_comIsDone)
	{
		if(pC->Status.flagNicComTimeoutError == true)
		{
			pC->Nic.mode.comStatus = NCS_comIsIdle;
			result = RES_NicComTimeout;
			return result;
		}
	}
	return result;
}
static eResult Control_WaitForNicFlagIsChanged(eBool* flag, eBool newstate, uint32_t timeout)
{
	eResult result = RES_OK;
	pC->Nic.mode.flagTime = 0;
	pC->Nic.mode.flagTimeout = timeout;
	pC->Nic.mode.flagWaitingForFlag = true;
	while(*flag != newstate)
	{
		pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemStatusComErrorFlags;
		NIC_StartComunication(1, 5000);
		result = Control_WaitForNicComunicationIsDone();
		if(result != RES_OK)
		{
			pC->Nic.mode.flagWaitingForFlag = false;
			return result;
		}
		if(pC->Status.flagNicFlagTimeoutError == true)
		{
			pC->Nic.mode.flagWaitingForFlag = false;
			result = RES_NicFlagTimeout;
			return result;
		}
	}
	pC->Nic.mode.flagWaitingForFlag = false;
	return result;
}
static eResult Control_ReadSystemInformationFromModule(void)
{
	eResult result = RES_OK;
	pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemInformation;
	NIC_StartComunication(1, 5000);
	result = Control_WaitForNicComunicationIsDone();
	return result;
}
static void Control_PrepareSystemConfigurationToWrite(void)
{
	pC->Nic.scWrite = pC->Nic.scDef;
	pC->Nic.scWrite.shifBaud = pC->Nic.scRead.shifBaud;
	pC->Nic.scWrite.flagsShifConf = pC->Nic.scRead.flagsShifConf;
	
	uint32_t idx = 0;
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.ssioType, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.ssioAddress, &idx, pC->Nic.scWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.scWrite.ssioBaud, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.ssioNumInBytes, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.ssioNumOutBytes, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.shifType, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.shifBaud, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.shifAddress, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.flagsShifConf, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.ssioNumBytesSsioIn, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.ssioNumBytesSsioOut, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.ssioOffsetAddressFbIn, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.ssioOffsetAddressFbOut, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.ssioWatchdogTime, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.ssioSwapShiftDir, &idx, pC->Nic.scWrite.regs);
	NIC_TableUint16ToTableUint16(pC->Nic.scWrite.reserved, pC->Nic.scWrite.regs, &idx, 4);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.numMappData, &idx, pC->Nic.scWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.scWrite.offsetAddressOutDataImage, &idx, pC->Nic.scWrite.regs);
	NIC_TableUint16ToTableUint16(pC->Nic.scWrite.mapData, pC->Nic.scWrite.regs, &idx, 78);
}
// Modbus RTU ************************************************
static void Control_PrepareStatusToSendModbusRtu(void)
{
	pC->Mbs.statusMbrtu[0] = pC->Outs.coils;
	if(pC->Mode.workType == workTypeRun)
		pC->Mbs.statusMbrtu[1] = 0x0001;
	else
		pC->Mbs.statusMbrtu[1] = 0x0000;
	pC->Mbs.statusMbrtu[2] = pC->Status.statBusMbrtuCounterError;
}
static eResult Control_ConfModbusRTU(void)
{
	eResult result = RES_OK;
	pC->Mbs.address = (uint8_t)pC->Ee.rData[EeAdd_mbrtuAddress];
	pC->Mbs.timeout	= pC->Ee.rData[EeAdd_mbrtuTimeout];
	pC->Mbs.parity = pC->Ee.rData[EeAdd_mbrtuParity];
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
	pC->Mbs.fun = MF_I;
	for(uint16_t i=0;i<MBS_HREGMAX;i++)
	{
		pC->Mbs.hregs[i] = 0x0000;
		pC->Mbs.iregs[i] = 0x0000;
	}
	MBS_ComConf();
	
	return result;
}
static void Control_RunModbusRTU(void)
{
	
}
static void Control_ErrorModbusRTU(void)
{
	
}
// Modbus TCP ************************************************
static void Control_PrepareStatusToSendModbusTCP(void)
{
	pC->Nic.cod.statusMbtcp[0] = pC->Outs.coils;
	if(pC->Mode.workType == workTypeRun)
		pC->Nic.cod.statusMbtcp[1] = 0x0001;
	else
		pC->Nic.cod.statusMbtcp[1] = 0x0000;
	pC->Nic.cod.statusMbtcp[2] = pC->Status.statBusMbtcpCounterError;
}
static eResult Control_CompareSystemInformationModbusTCP(void)
{
	eResult result = RES_OK;

	if(pC->Nic.si.devNumber != pC->Nic.siDefMb.devNumber)
		result = RES_NicSiIncompatible;
	else
		pC->Status.flagBusMbtcpCorrectDevNumber = true;
	
	if(pC->Nic.si.devClass 	!= pC->Nic.siDefMb.devClass)
		result = RES_NicSiIncompatible;
	else
		pC->Status.flagBusMbtcpCorrectDevClass = true;
	
	if(pC->Nic.si.protClass != pC->Nic.siDefMb.protClass)
		result = RES_NicSiIncompatible;
	else
		pC->Status.flagBusMbtcpCorrectProtClass = true;
	
	for(uint8_t i=0;i<64;i++)
		if(pC->Nic.si.firmName[i] != pC->Nic.siDefMb.firmName[i])
		{
			result = RES_NicSiIncompatible;
			break;
		}
	if(result == RES_OK)
		pC->Status.flagBusMbtcpCorrectFirmName = true;
	
	if(pC->Status.flagBusMbtcpCorrectDevNumber && pC->Status.flagBusMbtcpCorrectDevClass && pC->Status.flagBusMbtcpCorrectProtClass && pC->Status.flagBusMbtcpCorrectFirmName)
		pC->Status.flagBusMbtcpCorrectModule = true;
	
	return result;
}
static eResult Control_ReadConfigurationFromModuleModbusTCP(void)
{
	eResult result = RES_OK;
	pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemConfiguration;
	pC->Nic.mode.tabFunToSend[1] = NIC_ReadNetworkConfigurationMb300_333;
	NIC_StartComunication(2, 5000);
	result = Control_WaitForNicComunicationIsDone();
	return result;
}
static void Control_PrepareConfigurationToWriteModbusTCP(void)
{
	pC->Nic.ncMbWrite = pC->Nic.ncMbDef;
	
	pC->Nic.ncMbWrite.wdgTimeout = pC->Ee.rData[EeAdd_mbtcpTimeout];
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
	pC->Nic.ncMbWrite.ethAddress[0] = pC->Nic.si.ethMACAddr[0];
	pC->Nic.ncMbWrite.ethAddress[1] = pC->Nic.si.ethMACAddr[1];
	pC->Nic.ncMbWrite.ethAddress[2] = pC->Nic.si.ethMACAddr[2];
	pC->Nic.ncMbWrite.ethAddress[3] = pC->Nic.si.ethMACAddr[3];
	pC->Nic.ncMbWrite.ethAddress[4] = pC->Nic.si.ethMACAddr[4];
	pC->Nic.ncMbWrite.ethAddress[5] = pC->Nic.si.ethMACAddr[5];
	

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
	for(uint32_t i=0;i<SC_REGMAX;i++)
	{
		if(pC->Nic.scRead.regs[i] != pC->Nic.scWrite.regs[i])
			result = RES_NicScIncompatible;
	}
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
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteSystemConfiguration;
	pC->Nic.mode.tabFunToSend[1] = NIC_WriteNetworkConfigurationMb300_333;
	pC->Nic.mode.tabFunToSend[2] = NIC_WriteClrcfgFlagInCommandFlags;
	pC->Nic.mode.tabFunToSend[3] = NIC_ReadSystemStatusComErrorFlags;
	NIC_StartComunication(4, 5000);
	result = Control_WaitForNicComunicationIsDone();
	if(result != RES_OK)
	{
		return result;
	}
	
	result = Control_WaitForNicFlagIsChanged(&pC->Nic.sscef.flagFlsCfg, false, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteStrcfgFlagInCommandFlags;
	pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemStatusComErrorFlags;
	NIC_StartComunication(2, 5000);
	result = Control_WaitForNicComunicationIsDone();
	if(result != RES_OK)
	{
		return result;
	}

	result = Control_WaitForNicFlagIsChanged(&pC->Nic.sscef.flagFlsCfg, false, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteInitFlagInCommandFlags;
	NIC_StartComunication(1, 5000);
	result = Control_WaitForNicComunicationIsDone();
	if(result != RES_OK)
	{
		return result;
	}
	
	return result;
}
static eResult Control_ConfModbusTCP(void)
{
	eResult result = RES_OK;
	
	result = Control_WaitForNicFlagIsChanged(&pC->Nic.sscef.flagReady, true, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	
	result = Control_ReadSystemInformationFromModule();
	if(result != RES_OK)
	{
		return result;
	}
	result = Control_CompareSystemInformationModbusTCP();
	if(result != RES_OK)
	{
		return result;
	}
	
	result = Control_ReadConfigurationFromModuleModbusTCP();
	if(result != RES_OK)
	{
		return result;
	}
	Control_PrepareSystemConfigurationToWrite();
	Control_PrepareConfigurationToWriteModbusTCP();
	result = Control_CompareConfigurationModbusTCP();
	if(result == RES_OK)
	{
		pC->Nic.mode.confStatus = NCS_confIsDone;
		return result;
	}
	
	result = Control_WriteConfigurationModbusTCP();
	if(result != RES_OK)
	{
		pC->Status.flagBusMbtcpConfigUploadError = true;
		return result;
	}
	
	result = Control_ReadConfigurationFromModuleModbusTCP();
	if(result != RES_OK)
	{
		pC->Status.flagBusMbtcpConfigUploadError = true;
		return result;
	}
	
	Control_PrepareConfigurationToWriteModbusTCP();
	result = Control_CompareConfigurationModbusTCP();
	if(result == RES_OK)
	{
		pC->Status.flagBusMbtcpConfigUploaded = true;
		pC->Nic.mode.confStatus = NCS_confIsDone;
		return result;
	}
	else
	{
		pC->Status.flagBusMbtcpConfigUploadError = true;
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
		pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemStatusComErrorFlags;
		pC->Nic.mode.tabFunToSend[2] = NIC_WriteStatusMb;
		NIC_StartComunication(3, 100);
	}
}
static void Control_ErrorModbusTCP(void)
{
	if(pC->Status.flagBusMbtcpCorrectModule == true)
	{
		if(pC->Nic.mode.comStatus == NCS_comIsDone)
			pC->Nic.mode.comStatus = NCS_comIsIdle;
		if(pC->Nic.mode.comStatus == NCS_comIsIdle)
		{
			pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemStatusComErrorFlags;
			pC->Nic.mode.tabFunToSend[1] = NIC_WriteStatusMb;
			NIC_StartComunication(2, 100);
		}
	}
}
// ProfiBUS **************************************************************
static void Control_PrepareStatusToSendProfiBUS(void)
{
	pC->Nic.cod.statusPfbus[0] = pC->Outs.coils;
	if(pC->Mode.workType == workTypeRun)
		pC->Nic.cod.statusPfbus[1] = 0x0001;
	else
		pC->Nic.cod.statusPfbus[1] = 0x0000;
	pC->Nic.cod.statusPfbus[2] = pC->Status.statBusPfbusCounterError;
}
static eResult Control_ComapreSystemInformationProfiBUS(void)
{
	eResult result = RES_OK;

	if(pC->Nic.si.devNumber != pC->Nic.siDefPfbus.devNumber)
		result = RES_NicSiIncompatible;
	else
		pC->Status.flagBusPfbusCorrectDevNumber = true;
	
	if(pC->Nic.si.devClass 	!= pC->Nic.siDefPfbus.devClass)
		result = RES_NicSiIncompatible;
	else
		pC->Status.flagBusPfbusCorrectDevClass = true;
	
	if(pC->Nic.si.protClass != pC->Nic.siDefPfbus.protClass)
		result = RES_NicSiIncompatible;
	else
		pC->Status.flagBusPfbusCorrectProtClass = true;
	
	for(uint8_t i=0;i<64;i++)
		if(pC->Nic.si.firmName[i] != pC->Nic.siDefPfbus.firmName[i])
		{
			result = RES_NicSiIncompatible;
			break;
		}
	if(result == RES_OK)
		pC->Status.flagBusPfbusCorrectFirmName = true;
	
	if(pC->Status.flagBusPfbusCorrectDevNumber && pC->Status.flagBusPfbusCorrectDevClass && pC->Status.flagBusPfbusCorrectProtClass && pC->Status.flagBusPfbusCorrectFirmName)
		pC->Status.flagBusPfbusCorrectModule = true;
	
	return result;
}
static eResult Control_ReadConfigurationFromModuleProfiBUS(void)
{
	eResult result = RES_OK;
	pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemConfiguration;
	pC->Nic.mode.tabFunToSend[1] = NIC_ReadNetworkConfigurationPfbus300_399;
	pC->Nic.mode.tabFunToSend[2] = NIC_ReadNetworkConfigurationPfbus400_430;
	NIC_StartComunication(3, 5000);
	result = Control_WaitForNicComunicationIsDone();
	return result;
}
static void Control_PrepareConfigurationToWriteProfiBUS(void)
{
	pC->Nic.ncPfbusWrite = pC->Nic.ncPfbusDef;
	
	pC->Nic.ncPfbusWrite.wdgTimeout = pC->Ee.rData[EeAdd_pfbusTimeout];
	pC->Nic.ncPfbusWrite.identNumber = pC->Ee.rData[EeAdd_pfbusIdentNumber];
	pC->Nic.ncPfbusWrite.stationAddress = pC->Ee.rData[EeAdd_pfbusAdress];
	pC->Nic.ncPfbusWrite.baudrate = pC->Ee.rData[EeAdd_pfbusBaudrate];
	
	pC->Nic.ncPfbusWrite.flagDpv1Enable = (eBool)pC->Ee.rData[EeAdd_pfbusDPV1Enable];
	pC->Nic.ncPfbusWrite.flagSyncSupperted = (eBool)pC->Ee.rData[EeAdd_pfbusSyncSupported];
	pC->Nic.ncPfbusWrite.flagFreezeSuported = (eBool)pC->Ee.rData[EeAdd_pfbusFreezeSupported];
	pC->Nic.ncPfbusWrite.flagFailSafeSuported = (eBool)pC->Ee.rData[EeAdd_pfbusFailSafeSupported];
	
	pC->Nic.ncPfbusWrite.flagsReg307 = 0x0000;
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagDpv1Enable << 0);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagSyncSupperted << 1);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagFreezeSuported << 2);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagFailSafeSuported << 3);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagAlarmSap50Deactivate << 4);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagIoDataSwap << 5);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagAutoConfiguration << 6);
	pC->Nic.ncPfbusWrite.flagsReg307 += (pC->Nic.ncPfbusWrite.flagAddressChangeNotAllowed << 7);
	
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
	for(uint32_t i=0;i<SC_REGMAX;i++)
	{
		if(pC->Nic.scRead.regs[i] != pC->Nic.scWrite.regs[i])
			result = RES_NicScIncompatible;
	}
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
	pC->Nic.mode.tabFunToSend[2] = NIC_WriteSystemConfiguration;
	pC->Nic.mode.tabFunToSend[3] = NIC_WriteClrcfgFlagInCommandFlags;
	pC->Nic.mode.tabFunToSend[4] = NIC_ReadSystemStatusComErrorFlags;
	NIC_StartComunication(5, 10000);
	result = Control_WaitForNicComunicationIsDone();
	if(result != RES_OK)
	{
		return result;
	}
	
	result = Control_WaitForNicFlagIsChanged(&pC->Nic.sscef.flagFlsCfg, false, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteStrcfgFlagInCommandFlags;
	pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemStatusComErrorFlags;
	NIC_StartComunication(2, 5000);
	result = Control_WaitForNicComunicationIsDone();
	if(result != RES_OK)
	{
		return result;
	}

	result = Control_WaitForNicFlagIsChanged(&pC->Nic.sscef.flagFlsCfg, false, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteInitFlagInCommandFlags;
	NIC_StartComunication(1, 5000);
	result = Control_WaitForNicComunicationIsDone();
	if(result != RES_OK)
	{
		return result;
	}
	
	return result;
}
static eResult Control_ConfProfiBUS(void)
{
	eResult result = RES_OK;
	
	result = Control_WaitForNicFlagIsChanged(&pC->Nic.sscef.flagReady, true, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	
	result = Control_ReadSystemInformationFromModule();
	if(result != RES_OK)
	{
		return result;
	}
	result = Control_ComapreSystemInformationProfiBUS();
	if(result != RES_OK)
	{
		return result;
	}

	result = Control_ReadConfigurationFromModuleProfiBUS();
	if(result != RES_OK)
	{
		return result;
	}
	Control_PrepareSystemConfigurationToWrite();
	Control_PrepareConfigurationToWriteProfiBUS();
	result = Control_CompareConfigurationProfiBUS();
	if(result == RES_OK)
	{
		pC->Nic.mode.confStatus = NCS_confIsDone;
		return result;
	}
	result = Control_WriteConfigurationProfiBUS();
	if(result != RES_OK)
	{
		pC->Status.flagBusPfbusConfigUploadError = true;
		return result;
	}
	
	result = Control_ReadConfigurationFromModuleProfiBUS();
	if(result != RES_OK)
	{
		pC->Status.flagBusPfbusConfigUploadError = true;
		return result;
	}
	Control_PrepareConfigurationToWriteProfiBUS();
	result = Control_CompareConfigurationProfiBUS();
	if(result == RES_OK)
	{
		pC->Status.flagBusPfbusConfigUploaded = true;
		pC->Nic.mode.confStatus = NCS_confIsDone;
		return result;
	}
	else
	{
		pC->Status.flagBusPfbusConfigUploadError = true;
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
		pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemStatusComErrorFlags;
		pC->Nic.mode.tabFunToSend[2] = NIC_WriteStatusPfbus;
		NIC_StartComunication(3, 100);
	}
}
static void Control_ErrorProfiBUS(void)
{
	if(pC->Status.flagBusPfbusCorrectModule == true)
	{
		if(pC->Nic.mode.comStatus == NCS_comIsDone)
			pC->Nic.mode.comStatus = NCS_comIsIdle;
		if(pC->Nic.mode.comStatus == NCS_comIsIdle)
		{
			pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemStatusComErrorFlags;
			pC->Nic.mode.tabFunToSend[1] = NIC_WriteStatusPfbus;
			NIC_StartComunication(2, 100);
		}
	}
}
// ProfiNET ************************************************************
static void Control_PrepareStatusToSendProfiNET(void)
{
	pC->Nic.cod.statusPfnet[0] = pC->Outs.coils;
	if(pC->Mode.workType == workTypeRun)
		pC->Nic.cod.statusPfnet[1] = 0x0001;
	else
		pC->Nic.cod.statusPfnet[1] = 0x0000;
	pC->Nic.cod.statusPfnet[2] = pC->Status.statBusPfnetCounterError;
}
static eResult Control_CompareSystemInformationProfiNET(void)
{
	eResult result = RES_OK;

	if(pC->Nic.si.devNumber != pC->Nic.siDefPfnet.devNumber)
		result = RES_NicSiIncompatible;
	else
		pC->Status.flagBusPfnetCorrectDevNumber = true;
	
	if(pC->Nic.si.devClass 	!= pC->Nic.siDefPfnet.devClass)
		result = RES_NicSiIncompatible;
	else
		pC->Status.flagBusPfnetCorrectDevClass = true;
	
	if(pC->Nic.si.protClass != pC->Nic.siDefPfnet.protClass)
		result = RES_NicSiIncompatible;
	else
		pC->Status.flagBusPfnetCorrectProtClass = true;
	
	for(uint8_t i=0;i<64;i++)
		if(pC->Nic.si.firmName[i] != pC->Nic.siDefPfnet.firmName[i])
		{
			result = RES_NicSiIncompatible;
			break;
		}
	if(result == RES_OK)
		pC->Status.flagBusPfnetCorrectFirmName = true;
	
	if(pC->Status.flagBusPfnetCorrectDevNumber && pC->Status.flagBusPfnetCorrectDevClass && pC->Status.flagBusPfnetCorrectProtClass && pC->Status.flagBusPfnetCorrectFirmName)
		pC->Status.flagBusPfnetCorrectModule = true;
	
	return result;
}
static eResult Control_ReadConfigurationFromModuleProfiNET(void)
{
	eResult result = RES_OK;
	pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemConfiguration;
	pC->Nic.mode.tabFunToSend[1] = NIC_ReadNetworkConfigurationPfnet300_399;
	pC->Nic.mode.tabFunToSend[2] = NIC_ReadNetworkConfigurationPfnet400_499;
	pC->Nic.mode.tabFunToSend[3] = NIC_ReadNetworkConfigurationPfnet500_598;
	pC->Nic.mode.tabFunToSend[4] = NIC_ReadNetworkConfigurationPfnet599_699;
	pC->Nic.mode.tabFunToSend[5] = NIC_ReadNetworkConfigurationPfnet700_799;
	pC->Nic.mode.tabFunToSend[6] = NIC_ReadNetworkConfigurationPfnet800_899;
	pC->Nic.mode.tabFunToSend[7] = NIC_ReadNetworkConfigurationPfnet900_987;
	NIC_StartComunication(8, 10000);
	result = Control_WaitForNicComunicationIsDone();
	return result;
}
static void Control_PrepareConfigurationToWriteProfiNET(void)
{
	pC->Nic.ncPfnetWrite = pC->Nic.ncPfnetDef;
	pC->Nic.ncPfnetWrite.wdgTimeout = pC->Ee.rData[EeAdd_pfnetTimeout];
	pC->Nic.ncPfnetWrite.vendorId = pC->Ee.rData[EeAdd_pfnetVendorId];
	pC->Nic.ncPfnetWrite.deviceId = pC->Ee.rData[EeAdd_pfnetDeviceId];
	
	pC->Nic.ncPfnetWrite.lengthNameOfStation = pC->Ee.rData[EeAdd_pfnetLengthNameOfStation];
	for(uint16_t i=0;i<240;i++)
		pC->Nic.ncPfnetWrite.nameOfStation[i] = pC->Ee.rData[EeAdd_pfnetNameOfStation + i];
	pC->Nic.ncPfnetWrite.lengthTypeOfStation = pC->Ee.rData[EeAdd_pfnetLengthTypeOfStation];
	for(uint16_t i=0;i<240;i++)
		pC->Nic.ncPfnetWrite.typeOfStation[i] = pC->Ee.rData[EeAdd_pfnetTypeOfStation + i];
		
	for(uint16_t i=0;i<28;i++)
		pC->Nic.ncPfnetWrite.deviceType[i] = pC->Ee.rData[EeAdd_pfnetDeviceType + i];
	
	for(uint16_t i=0;i<20;i++)
		pC->Nic.ncPfnetWrite.orderId[i] = pC->Ee.rData[EeAdd_pfnetOrderId + i];
	
	pC->Nic.ncPfnetWrite.ipAddress[0] = pC->Ee.rData[EeAdd_pfnetIP0];
	pC->Nic.ncPfnetWrite.ipAddress[1] = pC->Ee.rData[EeAdd_pfnetIP1];
	pC->Nic.ncPfnetWrite.ipAddress[2] = pC->Ee.rData[EeAdd_pfnetIP2];
	pC->Nic.ncPfnetWrite.ipAddress[3] = pC->Ee.rData[EeAdd_pfnetIP3];
	pC->Nic.ncPfnetWrite.subnetMask[0] = pC->Ee.rData[EeAdd_pfnetMask0];
	pC->Nic.ncPfnetWrite.subnetMask[1] = pC->Ee.rData[EeAdd_pfnetMask1];
	pC->Nic.ncPfnetWrite.subnetMask[2] = pC->Ee.rData[EeAdd_pfnetMask2];
	pC->Nic.ncPfnetWrite.subnetMask[3] = pC->Ee.rData[EeAdd_pfnetMask3];
	pC->Nic.ncPfnetWrite.gateway[0] = pC->Ee.rData[EeAdd_pfnetGateway0];
	pC->Nic.ncPfnetWrite.gateway[1] = pC->Ee.rData[EeAdd_pfnetGateway1];
	pC->Nic.ncPfnetWrite.gateway[2] = pC->Ee.rData[EeAdd_pfnetGateway2];
	pC->Nic.ncPfnetWrite.gateway[3] = pC->Ee.rData[EeAdd_pfnetGateway3];
	
	pC->Nic.ncPfnetWrite.hardwareRevision = pC->Ee.rData[EeAdd_pfnetHardwareRevision];
	pC->Nic.ncPfnetWrite.softwareRevision1 = pC->Ee.rData[EeAdd_pfnetSoftwareRevision1];
	pC->Nic.ncPfnetWrite.softwareRevision2 = pC->Ee.rData[EeAdd_pfnetSoftwareRevision2];
	pC->Nic.ncPfnetWrite.softwareRevision3 = pC->Ee.rData[EeAdd_pfnetSoftwareRevision3];
	pC->Nic.ncPfnetWrite.softwareRevisionPrefix = pC->Ee.rData[EeAdd_pfnetSoftwareRevisionPrefix];

	uint32_t idx = 0;
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.length, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.busStartup, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.wdgTimeout, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.vendorId, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.deviceId, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.maxAR, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.inputBytes, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.outputBytes, &idx, pC->Nic.ncPfnetWrite.regs);
	
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.lengthNameOfStation, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetWrite.nameOfStation, pC->Nic.ncPfnetWrite.regs, &idx, 240);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.lengthTypeOfStation, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetWrite.typeOfStation, pC->Nic.ncPfnetWrite.regs, &idx, 240);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetWrite.deviceType, pC->Nic.ncPfnetWrite.regs, &idx, 28);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetWrite.orderId, pC->Nic.ncPfnetWrite.regs, &idx, 20);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetWrite.ipAddress, pC->Nic.ncPfnetWrite.regs, &idx, 4);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetWrite.subnetMask, pC->Nic.ncPfnetWrite.regs, &idx, 4);
	NIC_TableUint8ToTableUint16(pC->Nic.ncPfnetWrite.gateway, pC->Nic.ncPfnetWrite.regs, &idx, 4);
	
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.hardwareRevision, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.softwareRevision1, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.softwareRevision2, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.softwareRevision3, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.softwareRevisionPrefix, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.maximumDiagRecords, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.instanceId, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.reserved1, &idx, pC->Nic.ncPfnetWrite.regs);
	
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.numApi, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.profileApi, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.numSubmoduleItem, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.slot, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.subslot, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.moduleId, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.subModuleId, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.provDataLen, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.consDataLen, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.dpmOffsetIn, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint32ToTableUint16(pC->Nic.ncPfnetWrite.dpmOffsetOut, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.offsetIopsProvider, &idx, pC->Nic.ncPfnetWrite.regs);
	NIC_Uint16ToTableUint16(pC->Nic.ncPfnetWrite.offsetIopsConsumer, &idx, pC->Nic.ncPfnetWrite.regs);
	
	NIC_TableUint16ToTableUint16(pC->Nic.ncPfnetWrite.reserved2, pC->Nic.ncPfnetWrite.regs, &idx, 4);
	NIC_TableUint16ToTableUint16(pC->Nic.ncPfnetWrite.structureSubmoduleOrApi, pC->Nic.ncPfnetWrite.regs, &idx, 365);
}
static eResult Control_CompareConfigurationProfiNET(void)
{
	eResult result = RES_OK;
	for(uint32_t i=0;i<SC_REGMAX;i++)
	{
		if(pC->Nic.scRead.regs[i] != pC->Nic.scWrite.regs[i])
			result = RES_NicScIncompatible;
	}
	for(uint32_t i=0;i<PFNET_REGMAX;i++)
	{
		if(pC->Nic.ncPfnetRead.regs[i] != pC->Nic.ncPfnetWrite.regs[i])
			result = RES_NicNcPfnetIncompatible;
	}
	return result;
}
static eResult Control_WriteConfigurationProfiNET(void)
{
	eResult result = RES_OK;
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteSystemConfiguration;
	pC->Nic.mode.tabFunToSend[1] = NIC_WriteNetworkConfigurationPfnet300_399;
	pC->Nic.mode.tabFunToSend[2] = NIC_WriteNetworkConfigurationPfnet400_499;
	pC->Nic.mode.tabFunToSend[3] = NIC_WriteNetworkConfigurationPfnet500_598;
	pC->Nic.mode.tabFunToSend[4] = NIC_WriteNetworkConfigurationPfnet599_699;
	pC->Nic.mode.tabFunToSend[5] = NIC_WriteNetworkConfigurationPfnet700_799;
	pC->Nic.mode.tabFunToSend[6] = NIC_WriteNetworkConfigurationPfnet800_899;
	pC->Nic.mode.tabFunToSend[7] = NIC_WriteNetworkConfigurationPfnet900_987;
	pC->Nic.mode.tabFunToSend[8] = NIC_WriteClrcfgFlagInCommandFlags;
	pC->Nic.mode.tabFunToSend[9] = NIC_ReadSystemStatusComErrorFlags;
	NIC_StartComunication(10, 10000);
	result = Control_WaitForNicComunicationIsDone();
	if(result != RES_OK)
	{
		return result;
	}
	
	result = Control_WaitForNicFlagIsChanged(&pC->Nic.sscef.flagFlsCfg, false, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteStrcfgFlagInCommandFlags;
	pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemStatusComErrorFlags;
	NIC_StartComunication(2, 5000);
	result = Control_WaitForNicComunicationIsDone();
	if(result != RES_OK)
	{
		return result;
	}

	result = Control_WaitForNicFlagIsChanged(&pC->Nic.sscef.flagFlsCfg, false, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	
	pC->Nic.mode.tabFunToSend[0] = NIC_WriteInitFlagInCommandFlags;
	NIC_StartComunication(1, 5000);
	result = Control_WaitForNicComunicationIsDone();
	if(result != RES_OK)
	{
		return result;
	}
	
	return result;
}
static eResult Control_ConfProfiNET(void)
{
	eResult result = RES_OK;

	result = Control_WaitForNicFlagIsChanged(&pC->Nic.sscef.flagReady, true, 10000);
	if(result != RES_OK)
	{
		return result;
	}
	
	result = Control_ReadSystemInformationFromModule();
	if(result != RES_OK)
	{
		return result;
	}
	result = Control_CompareSystemInformationProfiNET();
	if(result != RES_OK)
	{
		return result;
	}
	
	result = Control_ReadConfigurationFromModuleProfiNET();
	if(result != RES_OK)
	{
		return result;
	}
	Control_PrepareSystemConfigurationToWrite();
	Control_PrepareConfigurationToWriteProfiNET();
	result = Control_CompareConfigurationProfiNET();
	if(result == RES_OK)
	{
		pC->Nic.mode.confStatus = NCS_confIsDone;
		return result;
	}
	
	result = Control_WriteConfigurationProfiNET();
	if(result != RES_OK)
	{
		pC->Status.flagBusPfnetConfigUploadError = true;
		return result;
	}
	
	result = Control_ReadConfigurationFromModuleProfiNET();
	if(result != RES_OK)
	{
		pC->Status.flagBusPfnetConfigUploadError = true;
		return result;
	}
	
	Control_PrepareConfigurationToWriteProfiNET();
	result = Control_CompareConfigurationProfiNET();
	if(result == RES_OK)
	{
		pC->Status.flagBusPfnetConfigUploaded = true;
		pC->Nic.mode.confStatus = NCS_confIsDone;
		return result;
	}
	else
	{
		pC->Status.flagBusPfnetConfigUploadError = true;
		return result;
	}
}
static void Control_RunProfiNET(void)
{
	if(pC->Nic.mode.comStatus == NCS_comIsDone)
		pC->Nic.mode.comStatus = NCS_comIsIdle;
	if(pC->Nic.mode.comStatus == NCS_comIsIdle)
	{
		pC->Nic.mode.tabFunToSend[0] = NIC_ReadCoils;
		pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemStatusComErrorFlags;
		pC->Nic.mode.tabFunToSend[2] = NIC_WriteStatusPfnet;
		
		NIC_StartComunication(3, 100);
	}
}
static void Control_ErrorProfiNET(void)
{
	if(pC->Status.flagBusPfnetCorrectModule == true)
	{
		if(pC->Nic.mode.comStatus == NCS_comIsDone)
			pC->Nic.mode.comStatus = NCS_comIsIdle;
		if(pC->Nic.mode.comStatus == NCS_comIsIdle)
		{
			pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemStatusComErrorFlags;
			pC->Nic.mode.tabFunToSend[1] = NIC_WriteStatusPfnet;
			
			NIC_StartComunication(2, 100);
		}
	}
}
//*******************************************************************************
void Control_WorkTypeConf(void)
{
	eResult result = RES_OK;
	pC->Mode.workType = workTypeStop;
	Control_RccSystemInit();
	SysTick_Config(100000);
	Control_RccConf();
	Control_LedConf();
	Control_StructConf();
	Control_AdcConf();
	MBS_ComConf();
	NIC_ComConf();
	Outputs_Conf();
	Control_WriteConfigToFlash();
	result = Control_ReadConfigFromFlash();
	if(result != RES_OK)
	{
		pC->Mode.workType = workTypeError;
		pC->Status.flagIncorrectConfigReadingFromFlash = true;
		Control_TimsConf();
		return;
	}
	
	pC->Mode.workType = workTypeConf;
	delay_ms(300);
	
	for(uint8_t i=0;i<3;i++)
	{
		if(pC->Mode.protocol == Prot_Mbrtu)
			result = Control_ConfModbusRTU();
		else if(pC->Mode.protocol == Prot_Mbtcp)
			result = Control_ConfModbusTCP();
		else if(pC->Mode.protocol == Prot_Pfbus)
			result = Control_ConfProfiBUS();
		else if(pC->Mode.protocol == Prot_Pfnet)
			result = Control_ConfProfiNET();
		
		pC->Status.flagNicComTimeoutError = false;
		if(result == RES_OK)
			break;
	}
	
	if(result == RES_OK)
		pC->Mode.workType = workTypeRun;
	else
		pC->Mode.workType = workTypeError;
	
	Control_TimsConf();
}
static void Control_WorkTypeRun(void)
{
	if(pC->Mode.protocol == Prot_Mbrtu)
	{
		Control_PrepareStatusToSendModbusRtu();
		Control_RunModbusRTU();
		Outputs_RunModbusRTU();
	}
	else if(pC->Mode.protocol == Prot_Mbtcp)
	{
		Control_PrepareStatusToSendModbusTCP();
		Control_RunModbusTCP();
		Outputs_RunModbusTCP();
	}
	else if(pC->Mode.protocol == Prot_Pfbus)
	{
		Control_PrepareStatusToSendProfiBUS();
		Control_RunProfiBUS();
		Outputs_RunProfiBUS();
	}
	else if(pC->Mode.protocol == Prot_Pfnet)
	{
		Control_PrepareStatusToSendProfiNET();
		Control_RunProfiNET();
		Outputs_RunProfiNET();
	}
}
static void Control_WorkTypeError(void)
{
	if(pC->Mode.protocol == Prot_Mbrtu)
	{
		Control_PrepareStatusToSendModbusRtu();
		Control_ErrorModbusRTU();
	}
	else if(pC->Mode.protocol == Prot_Mbtcp)
	{
		Control_PrepareStatusToSendModbusTCP();
		Control_ErrorModbusTCP();
	}
	else if(pC->Mode.protocol == Prot_Pfbus)
	{
		Control_PrepareStatusToSendProfiBUS();
		Control_ErrorProfiBUS();
	}
	else if(pC->Mode.protocol == Prot_Pfnet)
	{
		Control_PrepareStatusToSendProfiNET();
		Control_ErrorProfiNET();
	}
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
