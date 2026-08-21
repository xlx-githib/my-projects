/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ov2640.h"
#include "dcmi_app.h"
#include "lcd.h"
#include "key.h"
#include "photo.h"
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_FSMC_Init();
  MX_USART1_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
	HAL_TIM_Base_Start_IT(&htim6);
	delay_Init();
	lcd_init();
	usart_rx_start();               /* 开启串口命令接收中断 */
	
	int8_t key = -1;
	photo_err_t err = PHOTO_OK;
	uint8_t scale = 1;              /* 1=Scale(全图缩放) 0=FullSize(1:1) */
	char msgbuf[16];                /* 缩放模式提示缓冲 */
	char cmd[16];                   /* 串口命令缓冲 */

	lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
	lcd_show_string(30, 70, 200, 16, 16, "OV2640 CAMERA", RED);
	lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
	
	while (ov2640_init())       
  {
      lcd_show_string(30, 130, 240, 16, 16, "OV2640 ERROR", RED);
      delay_ms(200);
  }

  lcd_show_string(30, 130, 200, 16, 16, "OV2640 OK", RED);

	/* SD 卡就绪检测: 挂载 + 建目录 + 定位下一个可用序号 */
	if (photo_sd_ready() == PHOTO_OK)
	{
		lcd_show_string(30, 150, 200, 16, 16, "SD OK", RED);
	}
	else
	{
		lcd_show_string(30, 150, 240, 16, 16, "SD ERROR", RED);
	}

	/* 提示文字需在预览启动前写, 预览(DMA直写LCD)启动后不能再用显示函数 */
	lcd_show_string(30, 170, 240, 16, 16, "Scale", RED);
	lcd_show_string(30, 190, 240, 16, 16, "KEY0:BMP KEY1:JPG", RED);
	lcd_show_string(30, 210, 240, 16, 16, "WK_UP:ZOOM", RED);

	photo_preview_rgb565();
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
		/* ====================== 串口命令处理 ======================
		 * 通过串口助手发送(以换行结尾):
		 *   JPEG  -> 拍一张 JPEG 照片并存 SD 卡
		 *   BMP   -> 拍一张 RGB565 照片并转存 BMP 文件
		 *   HELP  -> 打印支持的命令
		 */
		if (usart_cmd_poll(cmd, sizeof(cmd)))
		{
			if (strcmp(cmd, "JPEG") == 0)
			{
				printf("CMD: JPEG capture\r\n");
				err = photo_capture_jpeg();
				if (err == PHOTO_OK) err = photo_store_to_sdcard();
				printf("RESULT: %s\r\n", photo_err_str(err));
				photo_preview_rgb565();
			}
			else if (strcmp(cmd, "BMP") == 0)
			{
				printf("CMD: BMP capture\r\n");
				err = photo_capture_rgb565();
				if (err == PHOTO_OK) err = photo_store_to_sdcard();
				printf("RESULT: %s\r\n", photo_err_str(err));
				photo_preview_rgb565();
			}
			else if (strcmp(cmd, "HELP") == 0)
			{
				printf("CMDS: JPEG / BMP / HELP\r\n");
			}
			else
			{
				printf("unknown cmd: %s\r\n", cmd);
			}
		}

		key = key_scan(0);

		if (key == KEY0_PRES)
		{
			dcmi_stop();                                    /* 停止预览, 安全写文字 */
			lcd_show_string(30, 190, 240, 16, 16, "BMP capturing...", RED);
			err = photo_capture_rgb565();
			if (err == PHOTO_OK) err = photo_store_to_sdcard();
			lcd_show_string(30, 170, 240, 16, 16, (char *)photo_err_str(err), RED);
			delay_ms(1200);
			photo_preview_rgb565();
		}
		else if (key == KEY1_PRES)
		{
			dcmi_stop();
			lcd_show_string(30, 190, 240, 16, 16, "JPEG capturing...", RED);
			err = photo_capture_jpeg();
			if (err == PHOTO_OK) err = photo_store_to_sdcard();
			lcd_show_string(30, 170, 240, 16, 16, (char *)photo_err_str(err), RED);
			delay_ms(1200);
			photo_preview_rgb565();
		}
		else if (key == WKUP_PRES)
		{
			dcmi_stop();
			scale = !scale;
			if (scale == 0)
			{
				/* FullSize 1:1: 从传感器窗口中间裁出 LCD 尺寸区域 */
				ov2640_image_win_set((OV2640_SENSOR_WIDTH - lcddev.width) / 2,
				                     (OV2640_SENSOR_HEIGHT - lcddev.height) / 2,
				                     lcddev.width, lcddev.height);
				ov2640_outsize_set(lcddev.width, lcddev.height);
				sprintf(msgbuf, "Full Size 1:1");
			}
			else
			{
				/* Scale: 全传感器窗口缩放输出到 LCD 尺寸 */
				ov2640_image_win_set(0, 0, OV2640_SENSOR_WIDTH, OV2640_SENSOR_HEIGHT);
				ov2640_outsize_set(lcddev.width, lcddev.height);
				sprintf(msgbuf, "Scale");
			}
			lcd_show_string(30, 170, 240, 16, 16, msgbuf, RED);
			delay_ms(800);
			photo_preview_rgb565();
		}

		delay_ms(10);
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
