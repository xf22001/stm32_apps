

/*================================================================
 *
 *
 *   文件名称：spi_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 10时30分48秒
 *   修改日期：2021年06月16日 星期三 09时51分45秒
 *   描    述：
 *
 *================================================================*/
#include "spi_txrx.h"

#include <string.h>

#include "os_utils.h"
#include "object_class.h"

static object_class_t *spi_class = NULL;

static void free_spi_info(spi_info_t *spi_info)
{
	if(spi_info == NULL) {
		return;
	}

	mutex_delete(spi_info->hspi_mutex);

	os_free(spi_info);
}

static spi_info_t *alloc_spi_info(SPI_HandleTypeDef *hspi)
{
	spi_info_t *spi_info = NULL;

	if(hspi == NULL) {
		return spi_info;
	}

	spi_info = (spi_info_t *)os_calloc(1, sizeof(spi_info_t));

	if(spi_info == NULL) {
		return spi_info;
	}

	spi_info->hspi = hspi;
	spi_info->hspi_mutex = mutex_create();

	return spi_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	spi_info_t *spi_info = (spi_info_t *)o;
	SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)ctx;

	if(spi_info->hspi == hspi) {
		ret = 0;
	}

	return ret;
}

spi_info_t *get_or_alloc_spi_info(SPI_HandleTypeDef *hspi)
{
	spi_info_t *spi_info = NULL;

	os_enter_critical();

	if(spi_class == NULL) {
		spi_class = object_class_alloc();
	}

	os_leave_critical();

	spi_info = (spi_info_t *)object_class_get_or_alloc_object(spi_class, object_filter, hspi, (object_alloc_t)alloc_spi_info, (object_free_t)free_spi_info);

	return spi_info;
}

int spi_tx_data(spi_info_t *spi_info, uint8_t *data, uint16_t size, uint32_t timeout)
{
	int ret = -1;
	HAL_StatusTypeDef status;

	mutex_lock(spi_info->hspi_mutex);
	status = HAL_SPI_Transmit(spi_info->hspi, data, size, timeout);
	mutex_unlock(spi_info->hspi_mutex);

	if(status == HAL_OK) {
		ret = 0;
	}

	return ret;
}
int spi_rx_data(spi_info_t *spi_info, uint8_t *data, uint16_t size, uint32_t timeout)
{
	int ret = -1;
	HAL_StatusTypeDef status;

	mutex_lock(spi_info->hspi_mutex);
	status = HAL_SPI_Receive(spi_info->hspi, data, size, timeout);
	mutex_unlock(spi_info->hspi_mutex);

	if(status == HAL_OK) {
		ret = 0;
	}

	return ret;
}

int spi_tx_rx_data(spi_info_t *spi_info, uint8_t *tx_data, uint8_t *rx_data, uint16_t size, uint32_t timeout)
{
	int ret = -1;
	HAL_StatusTypeDef status;

	mutex_lock(spi_info->hspi_mutex);
	status = HAL_SPI_TransmitReceive(spi_info->hspi, tx_data, rx_data, size, timeout);
	mutex_unlock(spi_info->hspi_mutex);

	if(status == HAL_OK) {
		ret = 0;
	}

	return ret;
}
