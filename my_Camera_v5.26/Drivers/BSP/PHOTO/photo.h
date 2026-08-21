#ifndef __PHOTO_H
#define __PHOTO_H

#include "stm32f4xx_hal.h"

/* ====================== 照片格式 ====================== */
#define PHOTO_FMT_RGB565    1       /* RGB565 照片(存成BMP文件) */
#define PHOTO_FMT_JPEG      2       /* JPEG   照片(存成JPG文件) */

/* ====================== 拍照分辨率 ====================== */
#define PHOTO_WIDTH         320                /* RGB565/BMP 拍照宽度 */
#define PHOTO_HEIGHT        240                /* RGB565/BMP 拍照高度 */
#define PHOTO_JPEG_WIDTH    640                /* JPEG 拍照宽度 */
#define PHOTO_JPEG_HEIGHT   480                /* JPEG 拍照高度 */
#define PHOTO_RGB565_SIZE   (PHOTO_WIDTH * PHOTO_HEIGHT * 2)   /* RGB565 一帧 = 153600 字节 */

/* ====================== 外部 SRAM 缓冲区布局 ======================
 * 外部 SRAM 共 512KB, 起始地址 0x68000000
 *   [0x68000000] JPEG 拍照缓冲      128KB
 *   [0x68020000] RGB565 原始帧缓冲  153600 字节
 *   [0x68045800] BMP 文件缓冲       54 + 320*240*3 = 230454 字节
 */
#define EXT_SRAM_BASE           0x68000000UL
#define PHOTO_JPEG_BUF_SIZE     (128 * 1024)
#define PHOTO_JPEG_BUF          ((uint8_t *)EXT_SRAM_BASE)
#define PHOTO_RGB565_BUF        ((uint8_t *)(EXT_SRAM_BASE + PHOTO_JPEG_BUF_SIZE))
#define PHOTO_BMP_BUF           ((uint8_t *)(EXT_SRAM_BASE + PHOTO_JPEG_BUF_SIZE + PHOTO_RGB565_SIZE))
#define PHOTO_BMP_SIZE          (54 + PHOTO_WIDTH * PHOTO_HEIGHT * 3)

/* JPEG/RGB565 流式采集使用的行缓冲大小(字), 双缓冲 */
#define PHOTO_LINE_SIZE         1024

/* 照片信息结构体 */
typedef struct
{
    uint8_t   format;      /* PHOTO_FMT_RGB565 / PHOTO_FMT_JPEG */
    uint8_t  *data;        /* 照片数据指针(指向外部SRAM) */
    uint32_t  len;         /* 有效数据长度(字节) */
} photo_t;

/* 统一错误码: 所有拍照/存储接口返回该类型, 便于上层反馈 */
typedef enum
{
    PHOTO_OK = 0,           /* 成功 */
    PHOTO_ERR_TIMEOUT,      /* 采集超时(无帧同步) */
    PHOTO_ERR_NO_JPEG,      /* 未找到有效 JPEG 数据(FF D8~FF D9) */
    PHOTO_ERR_SD_MOUNT,     /* SD 卡挂载/建目录失败 */
    PHOTO_ERR_SD_OPEN,      /* 打开文件失败 */
    PHOTO_ERR_SD_WRITE,     /* 写文件失败 */
} photo_err_t;

extern photo_t g_photo;

uint8_t      photo_preview_rgb565(void);              /* 进入 RGB565 实时预览(LCD显示) */
photo_err_t  photo_capture_rgb565(void);              /* 拍一张 RGB565, 并生成BMP文件数据 */
photo_err_t  photo_capture_jpeg(void);                /* 拍一张 JPEG */
uint32_t     photo_make_bmp(uint8_t *dst, const uint8_t *src);   /* RGB565 转 24bit BMP */
photo_err_t  photo_store_to_sdcard(void);             /* 照片存SD卡 */
photo_err_t  photo_sd_ready(void);                    /* SD 卡就绪检测(挂载+建目录+序号定位) */
const char  *photo_err_str(photo_err_t err);          /* 错误码转字符串 */

#endif
