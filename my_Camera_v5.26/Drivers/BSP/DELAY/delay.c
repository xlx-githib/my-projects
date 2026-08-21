#include "delay.h"

void delay_Init(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_ms(uint32_t ms)
{
	uint32_t start_tick = DWT->CYCCNT;
	uint32_t delay_ticks = ms * (SystemCoreClock / 1000);
	while(DWT->CYCCNT - start_tick < delay_ticks)
	{
		__NOP();//½µµÍ¹¦ºÄ
	}
	return ;
}

void delay_us(uint32_t us)
{
	uint32_t start_tick = DWT->CYCCNT;
	uint32_t delay_ticks = us * (SystemCoreClock / 1000000);
	while(DWT->CYCCNT - start_tick < delay_ticks)
	{
		__NOP();//½µµÍ¹¦ºÄ
	}
	return ;
}
	
void delay_s(uint32_t s)
{
	uint32_t start_tick = DWT->CYCCNT;
	uint32_t delay_ticks = s * SystemCoreClock;
	while(DWT->CYCCNT - start_tick < delay_ticks)
	{
		__NOP();//½µµÍ¹¦ºÄ
	}
	return ;
}

