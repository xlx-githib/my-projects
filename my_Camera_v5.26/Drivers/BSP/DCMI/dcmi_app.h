#ifndef __DCMI_APP_H
#define __DCMI_APP_H

#include "lcd.h"
#include "dcmi.h"

extern DMA_HandleTypeDef hdma_dcmi;

void dcmi_start(void);
void dcmi_stop(void);
void dcmi_gpio_init(void);
void dcmi_dma_init(uint32_t mem0addr, uint32_t mem1addr, uint16_t memsize, uint32_t memblen, uint32_t meminc);

#endif
