#include "dcmi_app.h"

void dcmi_start(void)
{
    lcd_set_cursor(0, 0);                   
    lcd_write_ram_prepare();                
    __HAL_DMA_ENABLE(&hdma_dcmi);   
    DCMI->CR |= DCMI_CR_CAPTURE;           
}

void dcmi_stop(void)
{ 
    DCMI->CR &= ~(DCMI_CR_CAPTURE);         

    while (DCMI->CR & 0X01);                

    __HAL_DMA_DISABLE(&hdma_dcmi);  
}


void dcmi_dma_init(uint32_t mem0addr, uint32_t mem1addr, uint16_t memsize, uint32_t memblen, uint32_t meminc)
{    
		__HAL_RCC_DMA2_CLK_ENABLE();                                        /* 使能DMA2时钟 */
    __HAL_LINKDMA(&hdcmi, DMA_Handle, hdma_dcmi);       /* 将DMA与DCMI联系起来 */
    __HAL_DMA_DISABLE_IT(&hdma_dcmi, DMA_IT_TC);                /* 先关闭DMA传输完成中断(待会再按需开启) */

    /* 对齐正点原子例程: 每次按实际需求重新配置 DMA 数据宽度/地址增量,
       因为预览(半字+地址不变)与流式采集(字+地址递增)参数完全不同,
       若不重新配置, HAL_DMA_Start/MultiBufferStart 会沿用旧参数导致花屏/丢数据。 */
    hdma_dcmi.Instance = DMA2_Stream1;
    hdma_dcmi.Init.Channel = DMA_CHANNEL_1;
    hdma_dcmi.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_dcmi.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_dcmi.Init.MemInc = meminc;
    hdma_dcmi.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_dcmi.Init.MemDataAlignment = memblen;
    hdma_dcmi.Init.Mode = DMA_CIRCULAR;
    hdma_dcmi.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_dcmi.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_dcmi.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    hdma_dcmi.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_dcmi.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_DeInit(&hdma_dcmi);
    HAL_DMA_Init(&hdma_dcmi);

    __HAL_UNLOCK(&hdma_dcmi);
    
    if (mem1addr == 0)  
    {
        HAL_DMA_Start(&hdma_dcmi, (uint32_t)&DCMI->DR, mem0addr, memsize);
    }
    else               
    {
        HAL_DMAEx_MultiBufferStart(&hdma_dcmi, (uint32_t)&DCMI->DR, mem0addr, mem1addr, memsize); 
        __HAL_DMA_ENABLE_IT(&hdma_dcmi, DMA_IT_TC);     
        HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 2, 3);         
        HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
    }
}

/**
 * @brief  Re-init DCMI GPIO (AF13)
 *         SDIO shares PC8/PC9/PC11 with DCMI, so after SD card access,
 *         DCMI GPIO must be restored before preview/capture again.
 */
void dcmi_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_DCMI_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;

    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_6;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

