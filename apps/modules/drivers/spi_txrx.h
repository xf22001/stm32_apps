

/*================================================================
 *   
 *   
 *   文件名称：spi_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 10时30分53秒
 *   修改日期：2021年06月16日 星期三 10时00分13秒
 *   描    述：
 *
 *================================================================*/
#ifndef _SPI_TXRX_H
#define _SPI_TXRX_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"
#include "os_utils.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	SPI_HandleTypeDef *hspi;
	os_mutex_t hspi_mutex;
} spi_info_t;

spi_info_t *get_or_alloc_spi_info(SPI_HandleTypeDef *hspi);
int spi_tx_data(spi_info_t *spi_info, uint8_t *data, uint16_t size, uint32_t timeout);
int spi_rx_data(spi_info_t *spi_info, uint8_t *data, uint16_t size, uint32_t timeout);
int spi_tx_rx_data(spi_info_t *spi_info, uint8_t *tx_data, uint8_t *rx_data, uint16_t size, uint32_t timeout);

#endif //_SPI_TXRX_H
