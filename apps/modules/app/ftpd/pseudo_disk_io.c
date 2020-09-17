

/*================================================================
 *
 *
 *   文件名称：pseudo_disk_io.c
 *   创 建 者：肖飞
 *   创建日期：2020年09月17日 星期四 09时40分42秒
 *   修改日期：2020年09月17日 星期四 15时13分50秒
 *   描    述：
 *
 *================================================================*/
#include "diskio.h"
#include "ff_gen_drv.h"

#include "os_utils.h"
#include "log.h"

#define SECTOR_SIZE 512
#define DATA_SIZE (64 * 1024)
#define DISK_SIZE (DATA_SIZE)

static char *disk = NULL;

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff)
{
	DSTATUS ret = RES_NOTRDY;

	if (pdrv != 0) {
		debug("\n");
		return ret;
	}

	ret = RES_PARERR;

	switch(cmd) {
		case CTRL_SYNC: {
			//sync disk;
			ret = RES_OK;
		}
		break;

		case GET_SECTOR_SIZE: {
			*((DWORD *)buff) = SECTOR_SIZE;
			ret = RES_OK;
		}
		break;

		case GET_SECTOR_COUNT: {
			*((DWORD *)buff) = DATA_SIZE / SECTOR_SIZE;
			ret = RES_OK;
		}
		break;

		case GET_BLOCK_SIZE: {
			*((WORD *)buff) = 1;
			ret = RES_OK;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

__weak DWORD get_fattime()
{
	// I am lazy.
	return 0;
}

DSTATUS disk_initialize(BYTE pdrv)
{
	DSTATUS ret = STA_NOINIT;

	if (pdrv != 0) {
		debug("\n");
		return ret;
	}

	ret = STA_NODISK;

	if(disk == NULL) {
		disk = (char *)os_alloc(DISK_SIZE);

		if(disk != NULL) {
			ret = RES_OK;
		} else {
			debug("\n");
		}
	} else {
		ret = RES_OK;
	}

	return ret;
}

DSTATUS disk_status (BYTE pdrv)
{
	DSTATUS ret = STA_NOINIT;

	if (pdrv != 0) {
		debug("\n");
		return ret;
	}

	ret = RES_OK;

	return ret;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
	DSTATUS ret = STA_NOINIT;
	size_t offset = SECTOR_SIZE * sector;
	size_t len = SECTOR_SIZE * count;

	if (pdrv != 0) {
		debug("\n");
		return ret;
	}

	ret = RES_NOTRDY;

	if((offset >= DISK_SIZE) || (offset + len > DISK_SIZE)) {
		debug("\n");
	} else {
		memcpy(disk + offset, buff, len);
		ret = RES_OK;
	}

	debug("offset:%d, len:%d, ret:%d\n", offset, len, ret);

	return ret;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
	DSTATUS ret = STA_NOINIT;
	size_t offset = SECTOR_SIZE * sector;
	size_t len = SECTOR_SIZE * count;

	if (pdrv != 0) {
		debug("\n");
		return ret;
	}

	ret = RES_NOTRDY;

	if((offset >= DISK_SIZE) || (offset + len >= DISK_SIZE)) {
		debug("\n");
	} else {
		memcpy(buff, disk + offset, len);
		ret = RES_OK;
	}

	debug("offset:%d, len:%d, ret:%d\n", offset, len, ret);

	return ret;
}

