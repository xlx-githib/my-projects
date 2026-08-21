#include "photo.h"
#include "ov2640.h"
#include "dcmi_app.h"
#include "dcmi.h"
#include "lcd.h"
#include "usart.h"
#include "tim.h"
#include "delay.h"
#include "sdio_sdcard.h"
#include "ff.h"
#include <stdio.h>

/* ====================== 全局变量 ====================== */
photo_t g_photo;                                    /* 当前照片信息 */

/* FatFs 文件系统对象 */
static FATFS g_fs;
static uint8_t g_fs_mounted = 0;
static uint16_t g_photo_index = 0;
uint8_t g_ov_frame = 0;                             /* 每秒帧数统计 */
volatile uint8_t g_uart_tx_done = 1;                /* 串口发送完成标志 */
void (*dcmi_rx_callback)(void) = NULL;              /* DMA 行缓冲收满回调 */

/* DMA 行缓冲双缓冲, 每个 1024 字 = 4096 字节 */
__ALIGNED(4) uint32_t g_dcmi_line_buf[2][PHOTO_LINE_SIZE];

/* 流式采集状态 */
static volatile uint8_t  g_stream_busy = 0;         /* 正在流式采集中 */
static volatile uint8_t  g_stream_done = 0;         /* 一帧采集完成 */
static volatile uint32_t g_stream_len  = 0;         /* 已采集字节数 */
static uint8_t  *g_stream_buf = NULL;               /* 目标大缓冲指针 */
static uint32_t  g_stream_max = 0;                  /* 目标缓冲上限(字节) */

/**
 * @brief  DMA 行缓冲收满回调(在 DMA2_Stream1_IRQHandler 中断中被调用)
 *         双缓冲循环模式下, 每填满一个行缓冲触发一次 TC 中断,
 *         把刚填满的行缓冲拷贝到外部 SRAM 大缓冲。
 */
void photo_dma_rx_callback(void)
{
    uint16_t i;
    uint32_t *dst;

    if (!g_stream_busy) return;

    /* 防越界 */
    if (g_stream_len + PHOTO_LINE_SIZE * 4 > g_stream_max) return;

    dst = (uint32_t *)(g_stream_buf + g_stream_len);
    if (DMA2_Stream1->CR & DMA_SxCR_CT)          /* CT=1: 当前目标为buf1, 刚填满的是buf0 */
    {
        for (i = 0; i < PHOTO_LINE_SIZE; i++) dst[i] = g_dcmi_line_buf[0][i];
    }
    else                                         /* CT=0: 当前目标为buf0, 刚填满的是buf1 */
    {
        for (i = 0; i < PHOTO_LINE_SIZE; i++) dst[i] = g_dcmi_line_buf[1][i];
    }
    g_stream_len += PHOTO_LINE_SIZE * 4;
}

/**
 * @brief  DCMI 帧中断回调
 *         一帧图像结束时触发, 关闭DMA并收走最后一段不完整的数据。
 */
void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi_x)
{
    uint16_t i;
    uint16_t rlen;
    uint32_t *dst;

    __HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_FRAMERI);
    g_ov_frame++;

    /* HAL_DCMI_IRQHandler 处理完帧中断后会关闭帧中断, 这里重新使能(对齐正点原子例程),
       否则帧中断只能触发一次, 帧率统计失效, 后续流式采集也会卡死。 */
    __HAL_DCMI_ENABLE_IT(&hdcmi, DCMI_IT_FRAME);

    if (!g_stream_busy) return;

    /* 关闭 DMA, 读取最后一个行缓冲里已收到的剩余数据 */
    __HAL_DMA_DISABLE(&hdma_dcmi);
    while (DMA2_Stream1->CR & DMA_SxCR_EN);

    rlen = PHOTO_LINE_SIZE - __HAL_DMA_GET_COUNTER(&hdma_dcmi);
    if (rlen > PHOTO_LINE_SIZE) rlen = PHOTO_LINE_SIZE;

    if (rlen && (g_stream_len + rlen * 4 <= g_stream_max))
    {
        dst = (uint32_t *)(g_stream_buf + g_stream_len);
        if (DMA2_Stream1->CR & DMA_SxCR_CT)      /* CT=1: 当前目标为buf1, 剩余数据在buf1 */
        {
            for (i = 0; i < rlen; i++) dst[i] = g_dcmi_line_buf[1][i];
        }
        else                                     /* CT=0: 剩余数据在buf0 */
        {
            for (i = 0; i < rlen; i++) dst[i] = g_dcmi_line_buf[0][i];
        }
        g_stream_len += rlen * 4;
    }

    g_stream_done = 1;
    g_stream_busy = 0;
}

/**
 * @brief  流式采集一帧图像到外部 SRAM
 * @param  buf     : 目标缓冲指针
 * @param  max_len : 目标缓冲上限(字节)
 * @retval PHOTO_OK 成功, PHOTO_ERR_TIMEOUT 超时
 */
static photo_err_t photo_stream_capture(uint8_t *buf, uint32_t max_len)
{
    uint32_t timeout = 0;

    dcmi_stop();                                   /* 先停止当前采集(预览) */

    g_stream_buf  = buf;
    g_stream_max  = max_len;
    g_stream_len  = 0;
    g_stream_done = 0;
    g_stream_busy = 1;

    dcmi_rx_callback = photo_dma_rx_callback;

    /* 配置 DMA: 行缓冲双缓冲 + 循环模式 */
    dcmi_dma_init((uint32_t)g_dcmi_line_buf[0], (uint32_t)g_dcmi_line_buf[1],
                  PHOTO_LINE_SIZE, DMA_MDATAALIGN_WORD, DMA_MINC_ENABLE);

    /* 清帧中断标志, 使能帧中断 */
    __HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_FRAMERI);
    __HAL_DCMI_ENABLE_IT(&hdcmi, DCMI_IT_FRAME);

    dcmi_start();                                  /* 启动 DCMI + DMA */

    while (!g_stream_done)
    {
        delay_ms(1);
        if (++timeout > 2000)                      /* 2 秒超时保护 */
        {
            dcmi_stop();
            g_stream_busy = 0;
            dcmi_rx_callback = NULL;
            return PHOTO_ERR_TIMEOUT;
        }
    }

    dcmi_stop();
    dcmi_rx_callback = NULL;
    g_stream_busy = 0;
    return PHOTO_OK;
}

/**
 * @brief  进入 RGB565 实时预览(显示到LCD)
 */
uint8_t photo_preview_rgb565(void)
{
    lcd_set_cursor(0, 0);
    lcd_write_ram_prepare();

    dcmi_gpio_init();          /* 恢复 DCMI GPIO(写SD卡后可能被SDIO占用) */

    ov2640_rgb565_mode();
    dcmi_dma_init((uint32_t)&LCD->LCD_RAM, 0, 1, DMA_MDATAALIGN_HALFWORD, DMA_MINC_DISABLE);
    ov2640_outsize_set(lcddev.width, lcddev.height);

    /* 对齐例程: 先 start 再 stop 再 start, 防止切回 RGB565 后首帧数据错位导致花屏 */
    dcmi_start();
    dcmi_stop();
    dcmi_start();
    return 0;
}

/**
 * @brief  把 RGB565 原始数据转换成 24 位 BMP 文件数据(自顶向下)
 * @param  dst : 目标缓冲(PHOTO_BMP_BUF)
 * @param  src : RGB565 原始数据
 * @retval BMP 文件总长度(字节)
 */
uint32_t photo_make_bmp(uint8_t *dst, const uint8_t *src)
{
    uint32_t i;
    uint32_t img_size = PHOTO_WIDTH * PHOTO_HEIGHT * 3;
    uint32_t file_size = 54 + img_size;
    uint32_t h = (uint32_t)(-(int32_t)PHOTO_HEIGHT);   /* 负高度: 自顶向下 */
    uint16_t rgb565;
    uint8_t  r, g, b;
    uint32_t off = 0;
    uint32_t pixel_total = PHOTO_WIDTH * PHOTO_HEIGHT;

    /* ---------- BITMAPFILEHEADER 14 字节 ---------- */
    dst[off++] = 'B';
    dst[off++] = 'M';
    dst[off++] = (uint8_t)(file_size & 0xFF);
    dst[off++] = (uint8_t)((file_size >> 8) & 0xFF);
    dst[off++] = (uint8_t)((file_size >> 16) & 0xFF);
    dst[off++] = (uint8_t)((file_size >> 24) & 0xFF);
    dst[off++] = 0; dst[off++] = 0;                /* bfReserved1 */
    dst[off++] = 0; dst[off++] = 0;                /* bfReserved2 */
    dst[off++] = 54; dst[off++] = 0; dst[off++] = 0; dst[off++] = 0;   /* bfOffBits */

    /* ---------- BITMAPINFOHEADER 40 字节 ---------- */
    dst[off++] = 40; dst[off++] = 0; dst[off++] = 0; dst[off++] = 0;    /* biSize */
    dst[off++] = (uint8_t)(PHOTO_WIDTH & 0xFF);
    dst[off++] = (uint8_t)((PHOTO_WIDTH >> 8) & 0xFF);
    dst[off++] = 0; dst[off++] = 0;                /* biWidth */
    dst[off++] = (uint8_t)(h & 0xFF);
    dst[off++] = (uint8_t)((h >> 8) & 0xFF);
    dst[off++] = (uint8_t)((h >> 16) & 0xFF);
    dst[off++] = (uint8_t)((h >> 24) & 0xFF);      /* biHeight(负值=自顶向下) */
    dst[off++] = 1; dst[off++] = 0;                /* biPlanes */
    dst[off++] = 24; dst[off++] = 0;               /* biBitCount = 24 */
    dst[off++] = 0; dst[off++] = 0; dst[off++] = 0; dst[off++] = 0;    /* biCompression = BI_RGB */
    dst[off++] = (uint8_t)(img_size & 0xFF);
    dst[off++] = (uint8_t)((img_size >> 8) & 0xFF);
    dst[off++] = (uint8_t)((img_size >> 16) & 0xFF);
    dst[off++] = (uint8_t)((img_size >> 24) & 0xFF);                    /* biSizeImage */
    dst[off++] = 0; dst[off++] = 0; dst[off++] = 0; dst[off++] = 0;    /* biXPelsPerMeter */
    dst[off++] = 0; dst[off++] = 0; dst[off++] = 0; dst[off++] = 0;    /* biYPelsPerMeter */
    dst[off++] = 0; dst[off++] = 0; dst[off++] = 0; dst[off++] = 0;    /* biClrUsed */
    dst[off++] = 0; dst[off++] = 0; dst[off++] = 0; dst[off++] = 0;    /* biClrImportant */

    /* ---------- 像素数据: RGB565 -> BGR 24bit ---------- */
    for (i = 0; i < pixel_total; i++)
    {
        /* 高字节在前(与LCD显示一致), 若颜色不对可交换下面两个字节 */
        rgb565 = ((uint16_t)src[i * 2] << 8) | src[i * 2 + 1];
        r = (uint8_t)((rgb565 >> 11) & 0x1F);
        g = (uint8_t)((rgb565 >> 5) & 0x3F);
        b = (uint8_t)(rgb565 & 0x1F);
        r = (uint8_t)((r << 3) | (r >> 2));
        g = (uint8_t)((g << 2) | (g >> 4));
        b = (uint8_t)((b << 3) | (b >> 2));
        dst[off++] = b;
        dst[off++] = g;
        dst[off++] = r;
    }
    return off;
}

/**
 * @brief  拍一张 RGB565 照片, 并生成 BMP 文件数据
 */
photo_err_t photo_capture_rgb565(void)
{
    photo_err_t ret;

    ov2640_rgb565_mode();
    ov2640_outsize_set(PHOTO_WIDTH, PHOTO_HEIGHT);

    ret = photo_stream_capture(PHOTO_RGB565_BUF, PHOTO_RGB565_SIZE);

    if (ret == PHOTO_OK)
    {
        g_photo.format = PHOTO_FMT_RGB565;
        g_photo.data   = PHOTO_BMP_BUF;
        g_photo.len    = photo_make_bmp(PHOTO_BMP_BUF, PHOTO_RGB565_BUF);
    }
    return ret;
}

/**
 * @brief  拍一张 JPEG 照片
 * @note   连续采集 3 帧, 丢弃前 2 帧, 取第 3 帧(OV2640 切换 JPEG 模式后
 *         前几帧可能不完整), 保证保存的照片可靠。
 */
photo_err_t photo_capture_jpeg(void)
{
    photo_err_t ret;
    uint32_t i, jpgstart = 0, jpglen = 0;
    uint8_t headok = 0;
    uint8_t *p;
    uint8_t frame;

    ov2640_jpeg_mode();
    ov2640_outsize_set(PHOTO_JPEG_WIDTH, PHOTO_JPEG_HEIGHT);

    /* 连续采 3 帧, 前 2 帧丢弃, 保留第 3 帧 */
    for (frame = 0; frame < 3; frame++)
    {
        ret = photo_stream_capture(PHOTO_JPEG_BUF, PHOTO_JPEG_BUF_SIZE);
        if (ret != PHOTO_OK) return ret;
    }

    /* 在缓冲中查找 FF D8 ~ FF D9, 提取有效 JPEG 数据 */
    p = PHOTO_JPEG_BUF;
    for (i = 0; i < g_stream_len - 1; i++)
    {
        if ((p[i] == 0xFF) && (p[i + 1] == 0xD8)) { jpgstart = i; headok = 1; }
        if ((p[i] == 0xFF) && (p[i + 1] == 0xD9) && headok)
        {
            jpglen = i - jpgstart + 2;
            break;
        }
    }

    if (jpglen)
    {
        g_photo.format = PHOTO_FMT_JPEG;
        g_photo.data   = PHOTO_JPEG_BUF + jpgstart;
        g_photo.len    = jpglen;
        return PHOTO_OK;
    }
    return PHOTO_ERR_NO_JPEG;
}

/**
 * @brief  把字符串里的十进制数字转成 uint16_t(用于解析文件名序号)
 */
static uint16_t photo_atoi(const char *s)
{
    uint16_t v = 0;
    while (*s >= '0' && *s <= '9')
    {
        v = v * 10 + (uint16_t)(*s - '0');
        s++;
    }
    return v;
}

/**
 * @brief  SD 卡就绪检测: 挂载文件系统、确保 PHOTO 目录存在,
 *         并扫描目录定位下一个可用序号(最大序号 + 1), 保证重启后不覆盖旧照片。
 */
photo_err_t photo_sd_ready(void)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    uint16_t max_index = 0;

    sdio_gpio_init();

    if (!g_fs_mounted)
    {
        res = f_mount(&g_fs, "0:", 1);
        if (res != FR_OK) return PHOTO_ERR_SD_MOUNT;
        g_fs_mounted = 1;
    }

    /* 确保 PHOTO 目录存在 */
    res = f_mkdir("0:PHOTO");
    if (res != FR_OK && res != FR_EXIST) return PHOTO_ERR_SD_MOUNT;

    /* 扫描目录, 找出 IMG_xxxx.* 中最大的序号 */
    res = f_opendir(&dir, "0:PHOTO");
    if (res == FR_OK)
    {
        while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0])
        {
            if (fno.fname[0] == 'I' && fno.fname[1] == 'M' &&
                fno.fname[2] == 'G' && fno.fname[3] == '_')
            {
                uint16_t idx = photo_atoi(&fno.fname[4]);
                if (idx > max_index) max_index = idx;
            }
        }
        f_closedir(&dir);
    }

    g_photo_index = max_index + 1;
    return PHOTO_OK;
}

/**
 * @brief  照片存储到 SD 卡(FatFs)
 * @note   文件命名: PHOTO/IMG_xxxx.JPG 或 PHOTO/IMG_xxxx.BMP,
 *         序号从 photo_sd_ready() 定位后开始, 每次保存后自增。
 */
photo_err_t photo_store_to_sdcard(void)
{
    FIL fil;
    UINT bw = 0;
    FRESULT res;
    char fname[24];

    /* 对齐例程 sw_sdcard_mode(): 写 SD 卡前把 PC8/PC9/PC11 从 DCMI(AF13)
       切回 SDIO(AF12), 否则第二次及之后写卡时引脚仍被 DCMI 占用, 读写失败。 */
    sdio_gpio_init();

    if (!g_fs_mounted)
    {
        res = f_mount(&g_fs, "0:", 1);
        if (res != FR_OK) return PHOTO_ERR_SD_MOUNT;
        g_fs_mounted = 1;
    }

    if (g_photo.format == PHOTO_FMT_JPEG)
        sprintf(fname, "0:PHOTO/IMG_%04d.JPG", g_photo_index);
    else
        sprintf(fname, "0:PHOTO/IMG_%04d.BMP", g_photo_index);

    res = f_open(&fil, fname, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) return PHOTO_ERR_SD_OPEN;

    res = f_write(&fil, g_photo.data, g_photo.len, &bw);
    f_close(&fil);
    if (res != FR_OK) return PHOTO_ERR_SD_WRITE;

    g_photo_index++;
    printf("saved %s len=%d\r\n", fname, bw);
    return PHOTO_OK;
}

/**
 * @brief  错误码转可读字符串
 */
const char *photo_err_str(photo_err_t err)
{
    switch (err)
    {
        case PHOTO_OK:           return "OK";
        case PHOTO_ERR_TIMEOUT:  return "CAP TIMEOUT";
        case PHOTO_ERR_NO_JPEG:  return "NO JPEG DATA";
        case PHOTO_ERR_SD_MOUNT: return "SD MOUNT FAIL";
        case PHOTO_ERR_SD_OPEN:  return "SD OPEN FAIL";
        case PHOTO_ERR_SD_WRITE: return "SD WRITE FAIL";
        default:                 return "UNKNOWN";
    }
}

/**
 * @brief  TIM6 周期中断回调(每秒打印一次帧率)
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        printf("frame:%dfps\r\n", g_ov_frame);
        g_ov_frame = 0;
    }
}

/**
 * @brief  串口发送完成回调
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        g_uart_tx_done = 1;
    }
}
