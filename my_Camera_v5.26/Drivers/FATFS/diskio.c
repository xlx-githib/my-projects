/*-----------------------------------------------------------------------/
/  Low level disk interface module for SD card (SDIO)                     /
/  对接 sdio_sdcard.c 底层驱动                                             /
/-----------------------------------------------------------------------*/

#include "ff.h"             /* 提供 BYTE/LBA_t/DSTATUS/DRESULT 类型 */
#include "diskio.h"
#include "sdio_sdcard.h"

static volatile DSTATUS g_sd_status = STA_NOINIT;

/**
 * @brief  初始化磁盘(SD 卡)
 */
DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv) return STA_NOINIT;

    if (SD_Init() == SD_OK)
    {
        g_sd_status = 0;
    }
    else
    {
        g_sd_status = STA_NOINIT;
    }
    return g_sd_status;
}

/**
 * @brief  获取磁盘状态
 */
DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv) return STA_NOINIT;
    return g_sd_status;
}

/**
 * @brief  读扇区
 */
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv || !count) return RES_PARERR;
    if (g_sd_status & STA_NOINIT) return RES_NOTRDY;

    if (SD_ReadBlocks((uint32_t *)buff, sector, 512, count) == SD_OK)
    {
        return RES_OK;
    }
    return RES_ERROR;
}

/**
 * @brief  写扇区
 */
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv || !count) return RES_PARERR;
    if (g_sd_status & STA_NOINIT) return RES_NOTRDY;

    if (SD_WriteBlocks((uint32_t *)buff, sector, 512, count) == SD_OK)
    {
        return RES_OK;
    }
    return RES_ERROR;
}

/**
 * @brief  磁盘控制
 */
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv) return RES_PARERR;

    switch (cmd)
    {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(DWORD *)buff = SD_GetSectorCount();
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD *)buff = 512;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1;
            return RES_OK;
    }
    return RES_PARERR;
}
