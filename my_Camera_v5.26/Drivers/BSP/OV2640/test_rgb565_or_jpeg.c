#include "test_rgb565_or_jpeg.h"
#include "photo.h"
#include "usart.h"
#include "delay.h"
#include <stdio.h>

/* JPEG 尺寸支持表 */
const uint16_t jpeg_img_size_tbl[][2] =
{
    160, 120,
    176, 144,
    320, 240,
    400, 240,
    352, 288,
    640, 480,
    800, 600,
    1024, 768,
    1280, 800,
    1280, 960,
    1440, 900,
    1280, 1024,
    1600, 1200,
};

const char *JPEG_SIZE_TBL[13] = {"QQVGA", "QCIF", "QVGA", "WGVGA", "CIF", "VGA", "SVGA", "XGA", "WXGA", "SVGA", "WXGA+", "SXGA", "UXGA"};

uint8_t test_choose(void)
{
    uint8_t key_val;

    key_val = key_scan(0);
    if (key_val == 1) return 1;
    else if (key_val == 2) return 2;
    return 0;
}

void test_mode(uint8_t ov_mode)
{
    if (ov_mode == 1) rgb565_test();
    else if (ov_mode == 2) jpeg_test();
}

/* LCD 实时预览(RGB565 直写 LCD) */
void rgb565_test(void)
{
    lcd_clear(BLACK);
    lcd_set_window(0, 0, lcddev.width, lcddev.height);
    lcd_write_ram_prepare();

    ov2640_rgb565_mode();
    dcmi_dma_init((uint32_t)&LCD->LCD_RAM, 0, 1, DMA_MDATAALIGN_HALFWORD, DMA_MINC_DISABLE);
    ov2640_outsize_set(lcddev.width, lcddev.height);
    dcmi_start();

    while (1)
    {
        delay_ms(10);
    }
}

/* JPEG 测试: 拍一张并通过串口发送 */
void jpeg_test(void)
{
    uint32_t i;

    if (photo_capture_jpeg() == 0)
    {
        printf("jpeg len:%d\r\n", g_photo.len);
        for (i = 0; i < g_photo.len; i++)
        {
            while ((USART1->SR & 0x40) == 0);
            USART1->DR = g_photo.data[i];
        }
    }
    else
    {
        printf("jpeg capture fail\r\n");
    }
}
