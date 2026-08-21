#include "sccb.h"
#include "delay.h"

void sccb_init(void)
{
	SCCB_SCL(1);
	SCCB_SDA(1);
}

void sccb_start(void)
{
	SCCB_SCL(1);
	SCCB_SDA(1);
	delay_us(50);
	SCCB_SDA(0);
	delay_us(50);
	SCCB_SCL(0);
}

void sccb_stop(void)
{
	SCCB_SCL(1);
	SCCB_SDA(0);
	delay_us(50);
	SCCB_SDA(1);
	delay_us(50);
}

void sccb_nack(void)
{
	delay_us(50);
	SCCB_SDA(1);
	SCCB_SCL(1);
	delay_us(50);
	SCCB_SCL(0);
	delay_us(50);
	SCCB_SDA(0);
	delay_us(50);
}

uint8_t sccb_write_byte(uint8_t data)
{
	uint8_t i;
	uint8_t ret;
	for(i = 0;i < 8;i++)
	{
		if(data & 0x80)SCCB_SDA(1);
		else SCCB_SDA(0);
		delay_us(50);
		SCCB_SCL(1);
		delay_us(50);
		SCCB_SCL(0);
		data <<= 1;
	}
	
	SCCB_SDA(1);
	delay_us(50);
	SCCB_SCL(1);
	delay_us(50);
	if(SCCB_READ_SDA())ret = 1;
	else ret = 0;
	SCCB_SCL(0);
	return ret;
}

uint8_t sccb_read_byte(void)
{
	uint8_t i;
	uint8_t receive = 0;
	for(i = 0;i < 8;i++)
	{
		receive <<= 1;
		SCCB_SCL(1);
		if(SCCB_READ_SDA())receive++;
		delay_us(50);
		SCCB_SCL(0);
		delay_us(50);
	}
	return receive;
}

