

/*================================================================
 *
 *
 *   文件名称：flash.c
 *   创 建 者：肖飞
 *   创建日期：2019年12月03日 星期二 10时09分30秒
 *   修改日期：2019年12月03日 星期二 13时35分37秒
 *   描    述：
 *
 *================================================================*/
#include "flash.h"

#include "app_platform.h"

int8_t flash_erase_sector(uint32_t start, uint32_t sectors)
{
	int8_t ret = 0;

	HAL_StatusTypeDef status;
	uint32_t sector;

	FLASH_EraseInitTypeDef init;

	status = HAL_FLASH_Unlock();

	if(HAL_OK != status) {
		ret = -1;
	}

	init.TypeErase = TYPEERASE_SECTORS;
	init.VoltageRange = VOLTAGE_RANGE_3;
	init.Sector = start;
	init.NbSectors = sectors;

	status = HAL_FLASHEx_Erase(&init, &sector);

	if(HAL_OK != status) {
		ret = -1;
	}

	status = HAL_FLASH_Lock();

	if(HAL_OK != status) {
		ret = -1;
	}

	return ret;
}

int8_t flash_write(uint32_t start_addr, uint8_t *data, uint32_t size)
{
	int8_t ret = 0;
	HAL_StatusTypeDef status;
	uint32_t i;

	status = HAL_FLASH_Unlock();

	if(HAL_OK != status) {
		ret = -1;
	}

	for(i = 0; i < size; i++) {
		status = HAL_FLASH_Program(TYPEPROGRAM_BYTE, start_addr + i, (uint64_t)(*(data + i)));

		if(HAL_OK != status) {
			ret = -1;
			break;
		}
	}

	status = HAL_FLASH_Lock();

	if(HAL_OK != status) {
		ret = -1;
	}

	return ret;
}

