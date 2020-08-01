#include "Control.h"
extern sControl* pC;
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
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
									RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;

	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

	RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_USART6EN;
}
static void Control_LedConf(void)
{
	LED_PORT->MODER 	|= GPIO_MODER_MODER8_0;
	LED_PORT->PUPDR 	|= GPIO_PUPDR_PUPDR8_0;
}
static void Control_TimsConf(void)
{
	TIM6->PSC = 84-1;
	TIM6->ARR = 10000;
	TIM6->DIER |= TIM_DIER_UIE;
	TIM6->CR1 |= TIM_CR1_CEN;
	NVIC_EnableIRQ(TIM6_DAC_IRQn);
}
static void Control_StructConf(void)
{
	pC->Mode.protocol = Prot_Mbtcp;
	pC->Mode.workType = workTypeRun;
	
	pC->Nic.mode.nicFun = NF_I;
	pC->Nic.mode.comStatus = NCS_isIdle;
	pC->Nic.mode.address = 2;
	pC->Nic.mode.timeout = 50;
	pC->Nic.mode.time = 0;
	pC->Nic.mode.tabFunToSendMb[0] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendMb[1] = NIC_ReadSystemInformation;
	pC->Nic.mode.tabFunToSendMb[2] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendMb[3] = NIC_ReadSystemConfiguration;
	pC->Nic.mode.tabFunToSendMb[4] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendMb[5] = NIC_ReadSystemStatusComErrorFlags;
	pC->Nic.mode.tabFunToSendMb[6] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendMb[7] = NIC_ReadNetworkStatusMb;
	pC->Nic.mode.tabFunToSendMb[8] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendMb[9] = NIC_ReadNetworkConfigurationMb;
	
	pC->Nic.mode.tabFunToSendPfbus[0] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendPfbus[1] = NIC_ReadSystemInformation;
	pC->Nic.mode.tabFunToSendPfbus[2] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendPfbus[3] = NIC_ReadSystemConfiguration;
	pC->Nic.mode.tabFunToSendPfbus[4] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendPfbus[5] = NIC_ReadSystemStatusComErrorFlags;
	pC->Nic.mode.tabFunToSendPfbus[6] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendPfbus[7] = NIC_ReadNetworkStatusPfbus;
	pC->Nic.mode.tabFunToSendPfbus[8] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendPfbus[9] = NIC_ReadNetworkConfigurationPfbus;
	
	pC->Nic.mode.tabFunToSendPfnet[0] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendPfnet[1] = NIC_ReadSystemInformation;
	pC->Nic.mode.tabFunToSendPfnet[2] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendPfnet[3] = NIC_ReadSystemConfiguration;
	pC->Nic.mode.tabFunToSendPfnet[4] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendPfnet[5] = NIC_ReadSystemStatusComErrorFlags;
	pC->Nic.mode.tabFunToSendPfnet[6] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendPfnet[7] = NIC_ReadNetworkStatusPfnet;
	pC->Nic.mode.tabFunToSendPfnet[8] = NIC_ReadCoils;
	pC->Nic.mode.tabFunToSendPfnet[9] = NIC_ReadNetworkConfigurationPfnet;
}
void Control_SystemStart(void)
{
	Control_RccSystemInit();
	SysTick_Config(100000);
	Control_RccConf();
	Control_LedConf();
//	Control_StructConf();
//	Control_TimsConf();
}
void delay_ms(uint32_t ms)
{
	pC->Mode.tick = 0;
	while(pC->Mode.tick < ms);
}
void SysTick_Handler(void)
{
	pC->Mode.tick++;
	if(pC->Nic.mode.comStatus != NCS_isIdle)
		pC->Nic.mode.time++;
}
static void Control_WorkTypeStop(void)
{
	Outputs_WorkTypeStop();
}
static void Control_WorkTypeRun(void)
{
	NIC_WorkTypeRunComunication();
	Outputs_WorkTypeRun();
}
static void Control_WorkTypeConfiguration(void)
{
	Outputs_WorkTypeConfiguration();
}
static void Control_WorkTypeError(void)
{
	Outputs_WorkTypeError();
}
static void Control_Act(void)
{
	if(pC->Mode.workType == workTypeStop)
	{
		Control_WorkTypeStop();
	}
	else if(pC->Mode.workType == workTypeRun)
	{
		Control_WorkTypeRun();
	}
	else if(pC->Mode.workType == workTypeConf)
	{
		Control_WorkTypeConfiguration();
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
