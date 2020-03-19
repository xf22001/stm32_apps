

/*================================================================
 *
 *
 *   文件名称：spi_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 10时30分48秒
 *   修改日期：2020年01月20日 星期一 10时55分03秒
 *   描    述：
 *
 *================================================================*/
#include "spi_txrx.h"

#include "os_utils.h"

static LIST_HEAD(spi_info_list);

spi_info_t *get_spi_info(SPI_HandleTypeDef *hspi)
{
	spi_info_t *spi_info = NULL;
	spi_info_t *spi_info_item = NULL;

	list_for_each_entry(spi_info_item, &spi_info_list, spi_info_t, list) {
		if(spi_info_item->hspi == hspi) {
			spi_info = spi_info_item;
			break;
		}
	}

	return spi_info;
}

void free_spi_info(spi_info_t *spi_info)
{
	if(spi_info == NULL) {
		return;
	}

	list_del(&spi_info->list);

	os_free(spi_info);
}

spi_info_t *alloc_spi_info(SPI_HandleTypeDef *hspi)
{
	spi_info_t *spi_info = NULL;

	spi_info = get_spi_info(hspi);

	if(spi_info != NULL) { return spi_info;
	}

	spi_info = (spi_info_t *)os_alloc(sizeof(spi_info_t));

	if(spi_info == NULL) {
		return spi_info;
	}

	spi_info->hspi = hspi;

	list_add_tail(&spi_info->list, &spi_info_list);

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
