#include "sdio_sdcard.h"
#include "delay.h"

uint8_t g_sd_type = 0;

static uint32_t g_sd_sector_count = 0;

/**
 * @brief  SDIO GPIO 初始化(AF12)
 *   PC8=SDIO_D0, PC9=SDIO_D1, PC10=SDIO_D2, PC11=SDIO_D3, PC12=SDIO_CK, PD2=SDIO_CMD
 *   注意: PC8/PC9/PC11 与 DCMI 复用, 使用 SD 卡时 DCMI 必须处于停止状态。
 *   写卡前需调用本函数把复用引脚切回 SDIO(AF12), 预览前再由 dcmi_gpio_init() 切回 DCMI。
 */
void sdio_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SDIO_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/**
 * @brief  设置 SDIO 分频系数, SDIO 时钟 = 48MHz / (clkdiv + 2)
 */
static void sdio_clock_set(uint8_t clkdiv)
{
    SDIO->CLKCR = (SDIO->CLKCR & ~SDIO_CLKCR_CLKDIV) | clkdiv;
}

/**
 * @brief  发送 SD 卡命令
 * @param  cmd      : 命令号
 * @param  arg      : 命令参数
 * @param  waitresp : 0=无响应 1=短响应(48bit) 2=长响应(136bit)
 * @retval SD_OK / SD_ERROR
 */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t waitresp)
{
    uint32_t timeout;
    uint32_t cmdreg;

    /* 等待命令线空闲 */
    timeout = 0xFFFFFF;
    while ((SDIO->STA & SDIO_STA_CMDACT) && timeout--) ;
    if (timeout == 0) return SD_ERROR;

    SDIO->ICR = 0xFFFFFFFF;                 /* 清中断状态 */

    SDIO->ARG = arg;

    cmdreg = SDIO_CMD_CPSMEN | SDIO_CMD_ENCMDCOMPL | (cmd & SDIO_CMD_CMDINDEX);
    if (waitresp == 1)       cmdreg |= SDIO_CMD_WAITRESP_0;   /* 短响应 */
    else if (waitresp == 2)  cmdreg |= SDIO_CMD_WAITRESP;     /* 长响应 */
    SDIO->CMD = cmdreg;

    /* 等待命令完成 */
    timeout = 0xFFFFFF;
    if (waitresp == 0)
    {
        while (!(SDIO->STA & (SDIO_STA_CMDSENT | SDIO_STA_CTIMEOUT)) && timeout--) ;
        if (timeout == 0) { SDIO->ICR = 0xFFFFFFFF; return SD_ERROR; }
    }
    else
    {
        while (!(SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL)) && timeout--) ;
        if (timeout == 0) { SDIO->ICR = 0xFFFFFFFF; return SD_ERROR; }
        if (SDIO->STA & (SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL))
        {
            SDIO->ICR = 0xFFFFFFFF;
            return SD_ERROR;
        }
    }

    SDIO->ICR = 0xFFFFFFFF;
    return SD_OK;
}

/**
 * @brief  SD 卡初始化
 * @retval SD_OK / SD_ERROR
 */
uint8_t SD_Init(void)
{
    uint8_t retry;
    uint32_t ocr, resp, rca, c_size;

    g_sd_type = SD_TYPE_ERR;
    g_sd_sector_count = 0;

    /* SDIO 时钟默认来自 48MHz(PLL48CLK, 由 PLLQ=7 输出 48MHz), 无需额外配置 */

    sdio_gpio_init();

    /* 复位 SDIO 外设 */
    __HAL_RCC_SDIO_FORCE_RESET();
    __HAL_RCC_SDIO_RELEASE_RESET();

    SDIO->CLKCR = 0;
    sdio_clock_set(118);                     /* 48MHz/(118+2) = 400kHz 低速识别 */
    SDIO->POWER = 0x03;                      /* 上电 */
    SDIO->CLKCR |= SDIO_CLKCR_CLKEN;         /* 使能 SDIO 时钟 */

    delay_ms(1);                             /* 等待 74 个以上时钟周期 */

    /* CMD0: GO_IDLE_STATE */
    if (sd_send_cmd(0, 0, 0)) return SD_ERROR;

    /* CMD8: SEND_IF_COND, 检测 V2.0 卡 */
    resp = 0;
    if (sd_send_cmd(8, 0x1AA, 1) == SD_OK)
    {
        resp = SDIO->RESP1;
    }

    if (resp == 0x1AA)                       /* V2.0 卡 */
    {
        retry = 0;
        do
        {
            if (sd_send_cmd(55, 0, 1)) return SD_ERROR;              /* CMD55: APP_CMD */
            if (sd_send_cmd(41, 0x40000000, 1)) return SD_ERROR;     /* ACMD41: HCS=1 */
            ocr = SDIO->RESP1;
            if (++retry > 200) return SD_ERROR;
        } while (!(ocr & 0x80000000));       /* 等待卡上电完成 */

        g_sd_type = (ocr & 0x40000000) ? SD_TYPE_V2HC : SD_TYPE_V2;
    }
    else                                     /* V1.x 或 MMC */
    {
        retry = 0;
        do
        {
            if (sd_send_cmd(55, 0, 1)) return SD_ERROR;
            if (sd_send_cmd(41, 0, 1)) return SD_ERROR;
            ocr = SDIO->RESP1;
            if (++retry > 200) return SD_ERROR;
        } while (!(ocr & 0x80000000));

        g_sd_type = SD_TYPE_V1;
    }

    /* CMD2: 读 CID */
    if (sd_send_cmd(2, 0, 2)) return SD_ERROR;

    /* CMD3: 读 RCA */
    if (sd_send_cmd(3, 0, 1)) return SD_ERROR;
    rca = SDIO->RESP1 >> 16;

    /* CMD9: 读 CSD, 计算容量 */
    if (sd_send_cmd(9, rca << 16, 2)) return SD_ERROR;
    if ((g_sd_type == SD_TYPE_V2) || (g_sd_type == SD_TYPE_V2HC))
    {
        c_size = ((SDIO->RESP2 & 0x3F) << 16) | (SDIO->RESP3 >> 16);
        g_sd_sector_count = (c_size + 1) * 1024;
    }
    else
    {
        c_size = (SDIO->RESP2 >> 8) & 0x0FFF;
        g_sd_sector_count = (c_size + 1) * 64;
    }

    /* CMD7: 选中卡 */
    if (sd_send_cmd(7, rca << 16, 1)) return SD_ERROR;

    /* ACMD6: 设置 4bit 总线宽度 */
    if (sd_send_cmd(55, rca << 16, 1)) return SD_ERROR;
    if (sd_send_cmd(6, 2, 1)) return SD_ERROR;
    SDIO->CLKCR = (SDIO->CLKCR & ~SDIO_CLKCR_WIDBUS) | SDIO_CLKCR_WIDBUS_0;   /* 4bit */

    /* 提高时钟到 16MHz */
    sdio_clock_set(1);

    return SD_OK;
}

/**
 * @brief  SD 卡多块读
 * @param  buf    : 数据缓冲(需 4 字节对齐)
 * @param  addr   : 扇区号(SDHC)或字节地址(SDSC, 内部自动换算)
 * @param  blksize: 块大小(通常 512)
 * @param  cnt    : 块数量
 * @retval SD_OK / SD_ERROR
 */
uint8_t SD_ReadBlocks(uint32_t *buf, uint64_t addr, uint32_t blksize, uint32_t cnt)
{
    uint32_t i, total_words;
    uint32_t timeout;

    total_words = (blksize * cnt) / 4;

    if (g_sd_type != SD_TYPE_V2HC)
    {
        addr = addr * blksize;               /* SDSC 卡使用字节地址 */
    }

    SDIO->ICR = 0xFFFFFFFF;
    SDIO->DTIMER = 0xFFFFFFFF;               /* 数据超时 */
    SDIO->DLEN = blksize * cnt;              /* 数据长度 */
    /* 块传输 + 读方向 + 512 字节块 */
    SDIO->DCTRL = SDIO_DCTRL_DTEN | SDIO_DCTRL_DTDIR | SDIO_DCTRL_DTMODE
                | SDIO_DCTRL_DBLOCKSIZE_0 | SDIO_DCTRL_DBLOCKSIZE_3;

    /* CMD17 单块读 / CMD18 多块读 */
    if (sd_send_cmd((cnt > 1) ? 18 : 17, (uint32_t)addr, 1)) return SD_ERROR;

    for (i = 0; i < total_words; i++)
    {
        timeout = 0xFFFFFF;
        while (!(SDIO->STA & SDIO_STA_RXDAVL))
        {
            if (SDIO->STA & (SDIO_STA_DTIMEOUT | SDIO_STA_DCRCFAIL | SDIO_STA_RXOVERR))
            {
                SDIO->ICR = 0xFFFFFFFF;
                return SD_ERROR;
            }
            if (--timeout == 0) { SDIO->ICR = 0xFFFFFFFF; return SD_ERROR; }
        }
        buf[i] = SDIO->FIFO;
    }

    /* 等待数据结束 */
    timeout = 0xFFFFFF;
    while (!(SDIO->STA & (SDIO_STA_DATAEND | SDIO_STA_DTIMEOUT | SDIO_STA_DCRCFAIL)) && timeout--) ;
    SDIO->ICR = 0xFFFFFFFF;
    if (timeout == 0 || (SDIO->STA & (SDIO_STA_DTIMEOUT | SDIO_STA_DCRCFAIL))) return SD_ERROR;

    if (cnt > 1) sd_send_cmd(12, 0, 1);      /* CMD12 停止多块读 */

    return SD_OK;
}

/**
 * @brief  SD 卡多块写
 * @param  buf    : 数据缓冲(需 4 字节对齐)
 * @param  addr   : 扇区号(SDHC)或字节地址(SDSC, 内部自动换算)
 * @param  blksize: 块大小(通常 512)
 * @param  cnt    : 块数量
 * @retval SD_OK / SD_ERROR
 */
uint8_t SD_WriteBlocks(uint32_t *buf, uint64_t addr, uint32_t blksize, uint32_t cnt)
{
    uint32_t i, total_words;
    uint32_t timeout;

    total_words = (blksize * cnt) / 4;

    if (g_sd_type != SD_TYPE_V2HC)
    {
        addr = addr * blksize;
    }

    SDIO->ICR = 0xFFFFFFFF;
    SDIO->DTIMER = 0xFFFFFFFF;
    SDIO->DLEN = blksize * cnt;
    /* 块传输 + 写方向 + 512 字节块 */
    SDIO->DCTRL = SDIO_DCTRL_DTEN | SDIO_DCTRL_DTMODE
                | SDIO_DCTRL_DBLOCKSIZE_0 | SDIO_DCTRL_DBLOCKSIZE_3;

    /* CMD24 单块写 / CMD25 多块写 */
    if (sd_send_cmd((cnt > 1) ? 25 : 24, (uint32_t)addr, 1)) return SD_ERROR;

    for (i = 0; i < total_words; i++)
    {
        timeout = 0xFFFFFF;
        while (!(SDIO->STA & SDIO_STA_TXFIFOHE))
        {
            if (SDIO->STA & (SDIO_STA_DTIMEOUT | SDIO_STA_DCRCFAIL | SDIO_STA_TXUNDERR))
            {
                SDIO->ICR = 0xFFFFFFFF;
                return SD_ERROR;
            }
            if (--timeout == 0) { SDIO->ICR = 0xFFFFFFFF; return SD_ERROR; }
        }
        SDIO->FIFO = buf[i];
    }

    /* 等待数据结束 */
    timeout = 0xFFFFFF;
    while (!(SDIO->STA & (SDIO_STA_DATAEND | SDIO_STA_DTIMEOUT | SDIO_STA_DCRCFAIL)) && timeout--) ;
    SDIO->ICR = 0xFFFFFFFF;
    if (timeout == 0 || (SDIO->STA & (SDIO_STA_DTIMEOUT | SDIO_STA_DCRCFAIL))) return SD_ERROR;

    if (cnt > 1) sd_send_cmd(12, 0, 1);      /* CMD12 停止多块写 */

    return SD_OK;
}

/**
 * @brief  获取 SD 卡总扇区数
 */
uint32_t SD_GetSectorCount(void)
{
    return g_sd_sector_count;
}
