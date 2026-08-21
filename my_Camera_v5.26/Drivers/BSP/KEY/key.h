#ifndef __KEY_H
#define __KEY_H

#include "main.h"
#include "delay.h"

#define KEY0        HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin)     /* 读取KEY0电平 */
#define KEY1        HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin)     /* 读取KEY1电平 */
#define WK_UP       HAL_GPIO_ReadPin(WK_UP_GPIO_Port, WK_UP_Pin)   /* 读取WK_UP电平 */

/* 键值定义 */
#define KEY0_PRES   1       /* KEY0 按下 */
#define KEY1_PRES   2       /* KEY1 按下 */
#define WKUP_PRES   4       /* WK_UP 按下 */

uint8_t key_scan(uint8_t mode);

#endif
