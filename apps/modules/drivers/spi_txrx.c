

/*================================================================
 *
 *
 *   文件名称：spi_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 10时30分48秒
 *   修改日期：2020年12月29日 星期二 15时06分08秒
 *   描    述：
 *
 *================================================================*/
#include "spi_txrx.h"
#include "os_utils.h"
#include "map_utils.h"

static map_utils_t *spi_map = NULL;

static spi_info_t *get_spi_info(SPI_HandleTypeDef *hspi)
{
	spi_info_t *spi_info = NULL;

	__disable_irq();
	spi_info = (spi_info_t *)map_utils_get_value(spi_map, hspi);
	__enable_irq();

	return spi_info;
}

void free_spi_info(spi_info_t *spi_info)
{
	int ret;

	if(spi_info == NULL) {
		return;
	}

	__disable_irq();
	ret = map_utils_remove_value(spi_map, spi_info->hspi);
	__enable_irq();

	if(ret != 0) {
	}

	os_free(spi_info);
}

spi_info_t *get_or_alloc_spi_info(SPI_HandleTypeDef *hspi)
{
	spi_info_t *spi_info = NULL;
	int ret;

	if(spi_map == NULL) {
		spi_map = map_utils_alloc(NULL);
	}

	spi_info = get_spi_info(hspi);

	if(spi_info != NULL) {
		return spi_info;
	}

	spi_info = (spi_info_t *)os_alloc(sizeof(spi_info_t));

	if(spi_info == NULL) {
		return spi_info;
	}

	spi_info->hspi = hspi;

	__disable_irq();
	ret = map_utils_add_key_value(spi_map, hspi, spi_info);
	__enable_irq();

	if(ret != 0) {
		free_spi_info(spi_info);
		spi_info = NULL;
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
