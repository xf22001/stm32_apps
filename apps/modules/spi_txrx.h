

/*================================================================
 *   
 *   
 *   文件名称：spi_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 10时30分53秒
 *   修改日期：2020年03月19日 星期四 13时44分21秒
 *   描    述：
 *
 *================================================================*/
#ifndef _SPI_TXRX_H
#define _SPI_TXRX_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "cmsis_os.h"
#include "app_platform.h"
#include "list_utils.h"

typedef struct {
	struct list_head list;
	SPI_HandleTypeDef *hspi;
} spi_info_t;

spi_info_t *get_spi_info(SPI_HandleTypeDef *hspi);
void free_spi_info(spi_info_t *spi_info);
spi_info_t *alloc_spi_info(SPI_HandleTypeDef *hspi);
int spi_tx_data(spi_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout);
int spi_rx_data(spi_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout);
#endif //_SPI_TXRX_H
