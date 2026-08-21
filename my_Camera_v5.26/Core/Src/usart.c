/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
int fputc(int ch, FILE *f)
{
    while ((USART1->SR & 0X40) == 0);               

    USART1->DR = (uint8_t)ch;                      
    return ch;
}

/* ====================== 串口命令接收 ====================== */
#define UART_RX_BUF_SIZE    64              /* 接收环形缓冲大小 */

static volatile uint8_t uart_rx_buf[UART_RX_BUF_SIZE];   /* 接收环形缓冲 */
static volatile uint8_t uart_rx_head = 0;                /* 写索引(中断中更新) */
static volatile uint8_t uart_rx_tail = 0;                /* 读索引(主循环更新) */
static uint8_t uart_rx_byte = 0;                         /* 单字节接收缓冲 */

/**
 * @brief  串口接收完成回调(每收到 1 字节触发)
 *         把字节存入环形缓冲, 并重新启动下一次接收。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint8_t next = (uint8_t)((uart_rx_head + 1) % UART_RX_BUF_SIZE);
        if (next != uart_rx_tail)                       /* 未满才写入, 满则丢弃 */
        {
            uart_rx_buf[uart_rx_head] = uart_rx_byte;
            uart_rx_head = next;
        }
        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1); /* 继续接收下一个字节 */
    }
}
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 921600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA2_Stream7;
    hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/**
 * @brief  开启串口接收中断(在 main 初始化阶段调用一次)
 */
void usart_rx_start(void)
{
    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
}

/**
 * @brief  从接收环形缓冲中提取一条完整命令
 * @param  cmd    : 输出命令缓冲
 * @param  maxlen : 命令缓冲最大长度(含结束符)
 * @retval 命令长度(>0 表示取到命令), 0 表示暂无完整命令
 * @note   命令以 '\r' 或 '\n' 结尾, 返回前统一转为大写, 便于比较。
 */
uint8_t usart_cmd_poll(char *cmd, uint8_t maxlen)
{
    uint8_t i = 0;

    while (uart_rx_tail != uart_rx_head && i < (maxlen - 1))
    {
        char ch = (char)uart_rx_buf[uart_rx_tail];
        uart_rx_tail = (uint8_t)((uart_rx_tail + 1) % UART_RX_BUF_SIZE);

        if (ch == '\r' || ch == '\n')       /* 行结束符 */
        {
            if (i > 0)                      /* 有内容, 结束本次命令 */
            {
                cmd[i] = '\0';
                return i;
            }
            continue;                       /* 空行忽略 */
        }

        if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 32);   /* 转大写 */
        cmd[i++] = ch;
    }

    return 0;
}

/* USER CODE END 1 */
