#ifndef __SCCB_H
#define __SCCB_H

#include "gpio.h"

#define SCCB_SCL(x)  do{x ?\
	HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, GPIO_PIN_SET):\
	HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, GPIO_PIN_RESET);\
}while(0)

#define SCCB_SDA(x) do{x ?\
	HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_SET):\
	HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_RESET);\
}while(0)

#define SCCB_READ_SDA() HAL_GPIO_ReadPin(SDA_GPIO_Port, SDA_Pin)

void sccb_init(void);
void sccb_start(void);
void sccb_stop(void);
uint8_t sccb_write_byte(uint8_t data);
uint8_t sccb_read_byte(void);
void sccb_nack(void);

#endif
