#ifndef __SDIO_SDCARD_H
#define __SDIO_SDCARD_H

#include "stm32f4xx_hal.h"

/* SD 卡类型 */
#define SD_TYPE_ERR     0
#define SD_TYPE_MMC     1
#define SD_TYPE_V1      2
#define SD_TYPE_V2      4
#define SD_TYPE_V2HC    6

/* 返回值 */
#define SD_OK           0
#define SD_ERROR        1

extern uint8_t g_sd_type;   /* SD 卡类型 */

void     sdio_gpio_init(void);                                          /* SDIO GPIO 复用切换(AF12), 写卡前调用 */
uint8_t  SD_Init(void);                                                 /* SD 卡初始化 */
uint8_t  SD_ReadBlocks(uint32_t *buf, uint64_t addr, uint32_t blksize, uint32_t cnt);   /* 多块读 */
uint8_t  SD_WriteBlocks(uint32_t *buf, uint64_t addr, uint32_t blksize, uint32_t cnt);  /* 多块写 */
uint32_t SD_GetSectorCount(void);                                       /* 获取总扇区数 */

#endif
