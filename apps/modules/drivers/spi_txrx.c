

/*================================================================
 *
 *
 *   文件名称：spi_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 10时30分48秒
 *   修改日期：2020年04月04日 星期六 18时53分12秒
 *   描    述：
 *
 *================================================================*/
#include "spi_txrx.h"

#include "os_utils.h"

static LIST_HEAD(spi_info_list);
static osMutexId spi_info_list_mutex = NULL;

static spi_info_t *get_spi_info(SPI_HandleTypeDef *hspi)
{
	spi_info_t *spi_info = NULL;
	spi_info_t *spi_info_item = NULL;
	osStatus os_status;

	if(spi_info_list_mutex == NULL) {
		return spi_info;
	}

	os_status = osMutexWait(spi_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(spi_info_item, &spi_info_list, spi_info_t, list) {
		if(spi_info_item->hspi == hspi) {
			spi_info = spi_info_item;
			break;
		}
	}

	os_status = osMutexRelease(spi_info_list_mutex);

	if(os_status != osOK) {
	}

	return spi_info;
}

void free_spi_info(spi_info_t *spi_info)
{
	osStatus os_status;

	if(spi_info == NULL) {
		return;
	}

	if(spi_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(spi_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&spi_info->list);

	os_status = osMutexRelease(spi_info_list_mutex);

	if(os_status != osOK) {
	}

	os_free(spi_info);
}

spi_info_t *get_or_alloc_spi_info(SPI_HandleTypeDef *hspi)
{
	spi_info_t *spi_info = NULL;
	osStatus os_status;

	spi_info = get_spi_info(hspi);

	if(spi_info != NULL) {
		return spi_info;
	}

	if(spi_info_list_mutex == NULL) {
		osMutexDef(spi_info_list_mutex);
		spi_info_list_mutex = osMutexCreate(osMutex(spi_info_list_mutex));

		if(spi_info_list_mutex == NULL) {
			return spi_info;
		}
	}

	spi_info = (spi_info_t *)os_alloc(sizeof(spi_info_t));

	if(spi_info == NULL) {
		return spi_info;
	}

	spi_info->hspi = hspi;

	os_status = osMutexWait(spi_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&spi_info->list, &spi_info_list);

	os_status = osMutexRelease(spi_info_list_mutex);

	if(os_status != osOK) {
	}

	return spi_info;
}

int spi_tx_data(spi_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout)
{
	int ret = -1;
	HAL_StatusTypeDef status = HAL_SPI_Transmit(info->hspi, data, size, timeout);

	if(status == HAL_OK) {
		ret = 0;
	}

	return ret;
}
int spi_rx_data(spi_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout)
{
	int ret = -1;
	HAL_StatusTypeDef status = HAL_SPI_Receive(info->hspi, data, size, timeout);

	if(status == HAL_OK) {
		ret = 0;
	}

	return ret;
}
