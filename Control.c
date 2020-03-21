#include "Control.h"
extern sControl* pC;
static void Control_RccSystemInit(void)
{
	uint32_t PLL_M=8, PLL_N=336, PLL_P=2, PLL_Q=7;
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
	//Zegary dla portów od A do I oraz DMA2
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
									RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
									RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN | RCC_AHB1ENR_GPIOIEN | RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
	//Zegary dla SPI2, SPI3
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN | RCC_APB1ENR_SPI3EN | RCC_APB1ENR_USART2EN | RCC_APB1ENR_USART3EN;
	//Zegary dla USART1, SPI1
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_SPI1EN | RCC_APB2ENR_USART6EN;
}
static void Control_LedConf(void)
{
//	LED_PORT->MODER 	|= GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;
//	LED_PORT->PUPDR 	|= GPIO_PUPDR_PUPDR12_0 | GPIO_PUPDR_PUPDR13_0 | GPIO_PUPDR_PUPDR14_0 | GPIO_PUPDR_PUPDR15_0;
	
	LED_PORT->MODER 	|= GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0;
	LED_PORT->PUPDR 	|= GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR3_0 | GPIO_PUPDR_PUPDR4_0 | GPIO_PUPDR_PUPDR5_0;
}
static void Control_TimsConf(void)
{
	TIM7->PSC = 84-1;
	TIM7->ARR = 10000;
	TIM7->DIER |= TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM7_IRQn);
}
void Control_SystemStart(void)
{
	Control_RccSystemInit();
	SysTick_Config(168000);
	Control_RccConf();
	Control_LedConf();
	Control_TimsConf();
}
void delay_ms(uint32_t ms)
{
	pC->Mode.tick = 0;
	while(pC->Mode.tick < ms);
}
void SysTick_Handler(void)
{
	pC->Mode.tick++;
	pC->Nic.time++;
}
static void Control_Stop(void)
{
	
}
static void Control_Run(void)
{
	
}
static void Control_Conf(void)
{
	
}
static void Control_Error(void)
{
	
}
static void Control_Act(void)
{
	if(pC->Mode.workType == workTypeStop)
	{
		Control_Stop();
	}
	else if(pC->Mode.workType == workTypeRun)
	{
		Control_Run();
	}
	else if(pC->Mode.workType == workTypeConf)
	{
		Control_Conf();
	}
	else if(pC->Mode.workType == workTypeError)
	{
		Control_Error();
	}
}
//***** Interrupts ********************************
void TIM7_IRQHandler(void)
{
	if((TIM7->SR & TIM_SR_UIF) != RESET)
	{
		Control_Act();
		TIM7->SR &= ~TIM_SR_UIF;
	}
}
