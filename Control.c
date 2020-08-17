#include "Control.h"
extern sControl* pC;
eBool correct = true;
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
	pC->Mode.protocol = Prot_Mbrtu;
	pC->Mode.workType = workTypeStop;
	
	for(uint32_t i=0;i<EE_VARMAX;i++)
		pC->Ee.VirtAddVarTab[i] = i;
	
	pC->Mode.ledTime = 0;
	pC->Mode.ledPeriod = 0;
	
	pC->Nic.mode.nicFun = NF_I;
	pC->Nic.mode.confStatus = NCS_confIsntDone;
	pC->Nic.mode.comStatus = NCS_comIsIdle;
	pC->Nic.mode.address = 2;
	pC->Nic.mode.timeout = 50;
	pC->Nic.mode.time = 0;
	pC->Nic.mode.numFunToSend = 0;
	pC->Nic.mode.maxFunToSend = NIC_FRAMEMAX;
	
//	pC->Nic.mode.tabFunToSend[0] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemInformation;
//	pC->Nic.mode.tabFunToSend[2] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSend[3] = NIC_ReadSystemConfiguration;
//	pC->Nic.mode.tabFunToSend[4] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSend[5] = NIC_ReadSystemStatusComErrorFlags;
//	pC->Nic.mode.tabFunToSend[6] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSend[7] = NIC_ReadNetworkStatusMb;
//	pC->Nic.mode.tabFunToSend[8] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSend[9] = NIC_ReadNetworkConfigurationMb;
//	
//	pC->Nic.mode.tabFunToSendMb[0] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendMb[1] = NIC_ReadSystemInformation;
//	pC->Nic.mode.tabFunToSendMb[2] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendMb[3] = NIC_ReadSystemConfiguration;
//	pC->Nic.mode.tabFunToSendMb[4] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendMb[5] = NIC_ReadSystemStatusComErrorFlags;
//	pC->Nic.mode.tabFunToSendMb[6] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendMb[7] = NIC_ReadNetworkStatusMb;
//	pC->Nic.mode.tabFunToSendMb[8] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendMb[9] = NIC_ReadNetworkConfigurationMb;
//	
//	pC->Nic.mode.tabFunToSendPfbus[0] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendPfbus[1] = NIC_ReadSystemInformation;
//	pC->Nic.mode.tabFunToSendPfbus[2] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendPfbus[3] = NIC_ReadSystemConfiguration;
//	pC->Nic.mode.tabFunToSendPfbus[4] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendPfbus[5] = NIC_ReadSystemStatusComErrorFlags;
//	pC->Nic.mode.tabFunToSendPfbus[6] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendPfbus[7] = NIC_ReadNetworkStatusPfbus;
//	pC->Nic.mode.tabFunToSendPfbus[8] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendPfbus[9] = NIC_ReadNetworkConfigurationPfbus;
//	
//	pC->Nic.mode.tabFunToSendPfnet[0] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendPfnet[1] = NIC_ReadSystemInformation;
//	pC->Nic.mode.tabFunToSendPfnet[2] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendPfnet[3] = NIC_ReadSystemConfiguration;
//	pC->Nic.mode.tabFunToSendPfnet[4] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendPfnet[5] = NIC_ReadSystemStatusComErrorFlags;
//	pC->Nic.mode.tabFunToSendPfnet[6] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendPfnet[7] = NIC_ReadNetworkStatusPfnet;
//	pC->Nic.mode.tabFunToSendPfnet[8] = NIC_ReadCoils;
//	pC->Nic.mode.tabFunToSendPfnet[9] = NIC_ReadNetworkConfigurationPfnet;
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
	pC->Ee.wData[EeAdd_stmProt] 					= (uint16_t)Prot_Mbtcp;
	
	pC->Ee.wData[EeAdd_mbrtuAddress] 			= 2;
	pC->Ee.wData[EeAdd_mbrtuTimeout] 			= 1000;
	pC->Ee.wData[EeAdd_mbrtuBaudrate] 		= 7;
	
	pC->Ee.wData[EeAdd_mbtcpTimeout] 			= 1234;
	pC->Ee.wData[EeAdd_mbtcpDataSwap] 		= 1;
	pC->Ee.wData[EeAdd_mbtcpIP0] 					= 13;
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
	pC->Ee.wData[EeAdd_mbtcpSerwerCons] 	= 16;
	pC->Ee.wData[EeAdd_mbtcpSendAckTimeoutLow] 			= (uint16_t)31000;
	pC->Ee.wData[EeAdd_mbtcpSendAckTimeoutHigh] 		= (uint16_t)(31000 >> 16);
	pC->Ee.wData[EeAdd_mbtcpConnectAckTimeoutLow] 	= (uint16_t)41000;
	pC->Ee.wData[EeAdd_mbtcpConnectAckTimeoutHigh] 	= (uint16_t)(41000 >> 16);
	pC->Ee.wData[EeAdd_mbtcpCloseAckTimeoutLow] 		= (uint16_t)51000;
	pC->Ee.wData[EeAdd_mbtcpCloseAckTimeoutHigh] 		= (uint16_t)(51000 >> 16);
	
	pC->Ee.wData[EeAdd_pfbusTimeout] 								= 1000;
	pC->Ee.wData[EeAdd_pfbusAdress] 								= 2;
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
void SysTick_Handler(void)
{
	pC->Mode.tick++;
	pC->Mode.ledTime++;
	if(pC->Nic.mode.comStatus != NCS_comIsIdle)
		pC->Nic.mode.time++;
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
//******************************************************
static eBool Control_ConfCheckModbusTCP(void)
{
	eBool correct = true;
	if(pC->Ee.rData[EeAdd_mbtcpTimeout] 	!= pC->Nic.ncMbRead.wdgTimeout)				correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpDataSwap] 	!= pC->Nic.ncMbRead.dataSwap)					correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpIP0] 			!= pC->Nic.ncMbRead.ipAddress[0])			correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpIP1] 			!= pC->Nic.ncMbRead.ipAddress[1])			correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpIP2] 			!= pC->Nic.ncMbRead.ipAddress[2])			correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpIP3] 			!= pC->Nic.ncMbRead.ipAddress[3])			correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpMask0] 		!= pC->Nic.ncMbRead.subnetMask[0])		correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpMask1] 		!= pC->Nic.ncMbRead.subnetMask[1])		correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpMask2] 		!= pC->Nic.ncMbRead.subnetMask[2])		correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpMask3] 		!= pC->Nic.ncMbRead.subnetMask[3])		correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpGateway0] 	!= pC->Nic.ncMbRead.gateway[0])				correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpGateway1] 	!= pC->Nic.ncMbRead.gateway[1])				correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpGateway2] 	!= pC->Nic.ncMbRead.gateway[2])				correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpGateway3] 	!= pC->Nic.ncMbRead.gateway[3])				correct = false;
	if(pC->Ee.rData[EeAdd_mbtcpSerwerCons]!= pC->Nic.ncMbRead.provSerwerConn)		correct = false;
	
	uint32_t SendAckTimeout = ((uint32_t)pC->Ee.rData[EeAdd_mbtcpSendAckTimeoutHigh]<<16) + ((uint32_t)pC->Ee.rData[EeAdd_mbtcpSendAckTimeoutLow]);
	uint32_t ConnectAckTimeout = ((uint32_t)pC->Ee.rData[EeAdd_mbtcpConnectAckTimeoutHigh]<<16) + ((uint32_t)pC->Ee.rData[EeAdd_mbtcpConnectAckTimeoutLow]);
	uint32_t CloseAckTimeout = ((uint32_t)pC->Ee.rData[EeAdd_mbtcpCloseAckTimeoutHigh]<<16) + ((uint32_t)pC->Ee.rData[EeAdd_mbtcpCloseAckTimeoutLow]);
	
	if(SendAckTimeout 		!= pC->Nic.ncMbRead.sendAckTimeout)		correct = false;
	if(ConnectAckTimeout 	!= pC->Nic.ncMbRead.conAckTimeout)		correct = false;
	if(CloseAckTimeout 		!= pC->Nic.ncMbRead.closeAckTimeout)	correct = false;
	return correct;
}
static void Control_ConfModbusRTU(void)
{
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
	pC->Mode.workType = workTypeRun;
}
static void Control_ConfModbusTCP(void)
{
	pC->flaga1 = 1; //*******************
	pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemInformation;
	pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemConfiguration;
	pC->Nic.mode.tabFunToSend[2] = NIC_ReadSystemStatusComErrorFlags;
	pC->Nic.mode.tabFunToSend[3] = NIC_ReadNetworkConfigurationMb;
	pC->Nic.mode.maxFunToSend = 4;
	NIC_StartComunication();
	while(pC->Nic.mode.comStatus != NCS_comIsDone){;}
	pC->Mode.workType = workTypeRun;
//	correct = Control_ConfCheckModbusTCP();
//	if(correct == true)
//	{
//		pC->Nic.mode.confStatus = NCS_confIsDone;
//		pC->Mode.workType = workTypeRun;
//		return;
//	}
//	else
//	{
//		pC->flaga1 = 2; //*******************
//		pC->Nic.mode.tabFunToSend[0] = NIC_WriteNetworkConfigurationMb;
//		pC->Nic.mode.tabFunToSend[1] = NIC_WriteClrcfgFlagInCommandFlags;
//		pC->Nic.mode.tabFunToSend[2] = NIC_ReadSystemStatusComErrorFlags;
//		pC->Nic.mode.maxFunToSend = 3;
//		NIC_StartComunication();
//		while(pC->Nic.mode.comStatus != NCS_comIsDone){;}
//		delay_ms(20000);
//		pC->flaga1 = 3; //*******************
//		while(pC->Nic.sscef.flagFlsCfg == true)
//		{
//			pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemStatusComErrorFlags;
//			pC->Nic.mode.maxFunToSend = 1;
//			NIC_StartComunication();
//			while(pC->Nic.mode.comStatus != NCS_comIsDone){;}
//		}
//		
//		pC->flaga1 = 4; //*******************
//		pC->Nic.mode.tabFunToSend[0] = NIC_WriteStrcfgFlagInCommandFlags;
//		pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemStatusComErrorFlags;
//		pC->Nic.mode.maxFunToSend = 2;
//		NIC_StartComunication();
//		while(pC->Nic.mode.comStatus != NCS_comIsDone){;}
//		delay_ms(20000);
//		pC->flaga1 = 5; //*******************
//		while(pC->Nic.sscef.flagFlsCfg == true)
//		{
//			pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemStatusComErrorFlags;
//			pC->Nic.mode.maxFunToSend = 1;
//			NIC_StartComunication();
//			while(pC->Nic.mode.comStatus != NCS_comIsDone){;}
//		}
//		
//		pC->flaga1 = 6; //*******************
//		pC->Nic.mode.tabFunToSend[0] = NIC_WriteInitFlagInCommandFlags;
//		pC->Nic.mode.maxFunToSend = 1;
//		NIC_StartComunication();
//		while(pC->Nic.mode.comStatus != NCS_comIsDone){;}
//	}
//	
//	pC->flaga1 = 7; //*******************
//	pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemInformation;
//	pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemConfiguration;
//	pC->Nic.mode.tabFunToSend[2] = NIC_ReadSystemStatusComErrorFlags;
//	pC->Nic.mode.tabFunToSend[3] = NIC_ReadNetworkConfigurationMb;
//	pC->Nic.mode.maxFunToSend = 4;
//	NIC_StartComunication();
//	while(pC->Nic.mode.comStatus != NCS_comIsDone){;}
//	
//	correct = Control_ConfCheckModbusTCP();
//	if(correct == true)
//	{
//		pC->flaga1 = 8; //*******************
//		pC->Nic.mode.confStatus = NCS_confIsDone;
//		pC->Mode.workType = workTypeRun;
//		return;
//	}
}

//static void Control_ConfModbusTCP(void)
//{
//	pC->Nic.mode.confStatus = NCS_confIsReading;
//	pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemInformation;
//	pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemConfiguration;
//	pC->Nic.mode.tabFunToSend[2] = NIC_ReadSystemStatusComErrorFlags;
//	pC->Nic.mode.tabFunToSend[3] = NIC_ReadNetworkStatusMb;
//	pC->Nic.mode.tabFunToSend[4] = NIC_ReadNetworkConfigurationMb;
//	pC->Nic.mode.maxFunToSend = 5;
//	NIC_StartComunication();
//	while(pC->Nic.mode.comStatus != NCS_comIsDone){;}
//	
//	pC->Nic.mode.confStatus = NCS_confIsChecking;
//	correct = Control_ConfCheckModbusTCP();
//	if(correct == true)
//	{
//		pC->Nic.mode.confStatus = NCS_confIsDone;
//		pC->Mode.workType = workTypeRun;
//	}
//	else
//	{
//		pC->Nic.mode.confStatus = NCS_confIsWriting;
//		pC->Nic.mode.tabFunToSend[0] = NIC_WriteNetworkConfigurationMb;
//		pC->Nic.mode.tabFunToSend[1] = NIC_WriteInitFlagInCommandFlags;
//		pC->Nic.mode.maxFunToSend = 2;
//		NIC_StartComunication();
//	}
//	while(pC->Nic.mode.comStatus != NCS_comIsDone){;}
//	
//	pC->Nic.mode.confStatus = NCS_confIsReading;
//	pC->Nic.mode.tabFunToSend[0] = NIC_ReadSystemInformation;
//	pC->Nic.mode.tabFunToSend[1] = NIC_ReadSystemConfiguration;
//	pC->Nic.mode.tabFunToSend[2] = NIC_ReadSystemStatusComErrorFlags;
//	pC->Nic.mode.tabFunToSend[3] = NIC_ReadNetworkStatusMb;
//	pC->Nic.mode.tabFunToSend[4] = NIC_ReadNetworkConfigurationMb;
//	pC->Nic.mode.maxFunToSend = 5;
//	NIC_StartComunication();
//	while(pC->Nic.mode.comStatus != NCS_comIsDone){;}
//	pC->Nic.mode.confStatus = NCS_confIsChecking;
//	correct = Control_ConfCheckModbusTCP();
//	if(correct == true)
//	{
//		pC->Nic.mode.confStatus = NCS_confIsDone;
//		pC->Mode.workType = workTypeRun;
//	}
//	else
//	{
//		//Error
//	}
//}
static void Control_ConfProfiBUS(void)
{
}
static void Control_ConfProfiNET(void)
{
}
static void Control_RunModbusRTU(void)
{
}
static void Control_RunModbusTCP(void)
{
	if(pC->Nic.mode.comStatus == NCS_comIsDone)
		pC->Nic.mode.comStatus = NCS_comIsIdle;
	if(pC->Nic.mode.comStatus == NCS_comIsIdle)
		NIC_StartComunication();
}
static void Control_RunProfiBUS(void)
{
	if(pC->Nic.mode.comStatus == NCS_comIsDone)
		pC->Nic.mode.comStatus = NCS_comIsIdle;
	if(pC->Nic.mode.comStatus == NCS_comIsIdle)
		NIC_StartComunication();
}
static void Control_RunProfiNET(void)
{
	if(pC->Nic.mode.comStatus == NCS_comIsDone)
		pC->Nic.mode.comStatus = NCS_comIsIdle;
	if(pC->Nic.mode.comStatus == NCS_comIsIdle)
		NIC_StartComunication();
}
static void Control_WorkTypeStop(void)
{
	Outputs_WorkTypeStop();
}
void Control_WorkTypeConf(void)
{
	pC->Mode.workType = workTypeConf;
	Outputs_WorkTypeConf();
	if(pC->Mode.protocol == Prot_Mbrtu)
	{
		Control_ConfModbusRTU();
	}
	else if(pC->Mode.protocol == Prot_Mbtcp)
	{
		Control_ConfModbusTCP();
	}
	else if(pC->Mode.protocol == Prot_Pfbus)
	{
		Control_ConfProfiBUS();
	}
	else if(pC->Mode.protocol == Prot_Pfnet)
	{
		Control_ConfProfiNET();
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
