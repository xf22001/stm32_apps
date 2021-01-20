

/*================================================================
 *
 *
 *   文件名称：spi_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 10时30分48秒
 *   修改日期：2021年01月20日 星期三 10时53分26秒
 *   描    述：
 *
 *================================================================*/
#include "spi_txrx.h"

#include <string.h>

#include "os_utils.h"
#include "map_utils.h"

static map_utils_t *spi_map = NULL;

static void free_spi_info(spi_info_t *spi_info)
{
	if(spi_info == NULL) {
		return;
	}

	os_free(spi_info);
}

static spi_info_t *alloc_spi_info(SPI_HandleTypeDef *hspi)
{
	spi_info_t *spi_info = NULL;

	if(hspi == NULL) {
		return spi_info;
	}

	spi_info = (spi_info_t *)os_alloc(sizeof(spi_info_t));

	if(spi_info == NULL) {
		return spi_info;
	}

	memset(spi_info, 0, sizeof(spi_info_t));

	spi_info->hspi = hspi;

	return spi_info;
}

spi_info_t *get_or_alloc_spi_info(SPI_HandleTypeDef *hspi)
{
	spi_info_t *spi_info = NULL;

	__disable_irq();

	if(spi_map == NULL) {
		spi_map = map_utils_alloc(NULL);
	}

	__enable_irq();

	spi_info = (spi_info_t *)map_utils_get_or_alloc_value(spi_map, hspi, (map_utils_value_alloc_t)alloc_spi_info, (map_utils_value_free_t)free_spi_info);

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
