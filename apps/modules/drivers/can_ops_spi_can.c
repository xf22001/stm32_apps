

/*================================================================
 *
 *
 *   文件名称：can_ops_spi_can.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月15日 星期二 20时36分26秒
 *   修改日期：2021年06月16日 星期三 11时48分18秒
 *   描    述：
 *
 *================================================================*/
#include "can_txrx.h"

#include "spi_txrx.h"
#include "drv_canfdspi_register.h"
#include "drv_canfdspi_defines.h"
#include "drv_canfdspi_api.h"

int8_t DRV_SPI_TransferData(uint8_t spiSlaveDeviceIndex, uint8_t *SpiTxData, uint8_t *SpiRxData, uint16_t spiTransferSize)
{
	int ret = -1;
	can_info_t *can_info = get_can_info_by_id(spiSlaveDeviceIndex);
	spi_info_t *spi_info;

	//OS_ASSERT(can_info != NULL);
	//OS_ASSERT(can_info->can_config->type == CAN_TYPE_SPI_CAN);

	spi_info = get_or_alloc_spi_info(can_info->hcan);

	HAL_GPIO_WritePin(can_info->can_config->spi_cs_port, can_info->can_config->spi_cs_pin, GPIO_PIN_RESET);
	ret = spi_tx_rx_data(spi_info, SpiTxData, SpiRxData, spiTransferSize, 10);
	HAL_GPIO_WritePin(can_info->can_config->spi_cs_port, can_info->can_config->spi_cs_pin, GPIO_PIN_SET);

	return ret;
}

static void _can_init(void *_can_info)
{
	can_info_t *can_info = (can_info_t *)_can_info;
	uint8_t test = 0;
	CAN_CONFIG config;
	CAN_TX_FIFO_CONFIG txConfig;
	CAN_RX_FIFO_CONFIG rxConfig;
	REG_CiFLTOBJ fObj;
	REG_CiMASK mObj;

	while(test != 0x01) {
		DRV_CANFDSPI_WriteByte(0, 0xE07, 0x01);
		DRV_CANFDSPI_ReadByte(0, 0xE07, &test);
	}

	// Reset device
	DRV_CANFDSPI_Reset(can_info->can_id);

	// Enable ECC and initialize RAM
	DRV_CANFDSPI_EccEnable(can_info->can_id);

	DRV_CANFDSPI_RamInit(can_info->can_id, 0xff);

	// Configure device
	DRV_CANFDSPI_ConfigureObjectReset(&config);
	config.IsoCrcEnable = 1;
	config.StoreInTEF = 0;

	DRV_CANFDSPI_Configure(can_info->can_id, &config);

	// Setup TX FIFO
	DRV_CANFDSPI_TransmitChannelConfigureObjectReset(&txConfig);
	txConfig.FifoSize = 7;
	txConfig.PayLoadSize = CAN_PLSIZE_8;
	txConfig.TxPriority = 1;

	DRV_CANFDSPI_TransmitChannelConfigure(can_info->can_id, can_info->can_config->tx_fifo, &txConfig);

	// Setup RX FIFO
	DRV_CANFDSPI_ReceiveChannelConfigureObjectReset(&rxConfig);
	rxConfig.FifoSize = 3;
	rxConfig.PayLoadSize = CAN_PLSIZE_8;

	DRV_CANFDSPI_ReceiveChannelConfigure(can_info->can_id, can_info->can_config->filter_fifo, &rxConfig);

	// Setup RX Filter
	fObj.word = 0;
	fObj.bF.SID = can_info->can_config->filter_id >> 18;
	fObj.bF.EID = can_info->can_config->filter_id;
	fObj.bF.EXIDE = can_info->can_config->filter_ext;

	DRV_CANFDSPI_FilterObjectConfigure(can_info->can_id, can_info->can_config->filter_number, &fObj.bF);

	// Setup RX Mask
	mObj.word = 0;
	mObj.bF.MSID = can_info->can_config->filter_mask_id >> 18;
	mObj.bF.MEID = can_info->can_config->filter_mask_id;
	mObj.bF.MIDE = can_info->can_config->filter_mask_ext;
	DRV_CANFDSPI_FilterMaskConfigure(can_info->can_id, can_info->can_config->filter_number, &mObj.bF);

	// Link FIFO and Filter
	DRV_CANFDSPI_FilterToFifoLink(can_info->can_id, can_info->can_config->filter_number, can_info->can_config->filter_fifo, true);

	// Setup Bit Time
	DRV_CANFDSPI_BitTimeConfigure(can_info->can_id, CAN_250K_2M, CAN_SSP_MODE_AUTO, CAN_SYSCLK_20M);

	// Setup Transmit and Receive Interrupts
	DRV_CANFDSPI_GpioModeConfigure(can_info->can_id, GPIO_MODE_INT, GPIO_MODE_INT);
	DRV_CANFDSPI_TransmitChannelEventEnable(can_info->can_id, can_info->can_config->tx_fifo, CAN_TX_FIFO_NOT_FULL_EVENT);
	DRV_CANFDSPI_ReceiveChannelEventEnable(can_info->can_id, can_info->can_config->filter_fifo, CAN_RX_FIFO_NOT_EMPTY_EVENT);
	DRV_CANFDSPI_ModuleEventEnable(can_info->can_id, CAN_TX_EVENT | CAN_RX_EVENT);

	// Select Normal ModeCAN_BITTIME_SETUP
	DRV_CANFDSPI_OperationModeSelect(can_info->can_id, CAN_NORMAL_MODE);
}

static int _can_tx_data(void *_can_info, can_tx_msg_t *msg, uint32_t timeout)
{
	int ret = -1;
	can_info_t *can_info = (can_info_t *)_can_info;
	uint32_t stamp = osKernelSysTick();
	HAL_StatusTypeDef status;
	CAN_TX_FIFO_EVENT event;
	CAN_TX_MSGOBJ txObj;

	txObj.bF.ctrl.DLC = msg->DLC;

	if(msg->IDE == CAN_ID_STD) {
		txObj.bF.ctrl.IDE = 0;
		txObj.bF.id.SID = msg->StdId;
	} else {
		txObj.bF.ctrl.IDE = 1;
		txObj.bF.id.SID = msg->ExtId >> 18;
		txObj.bF.id.EID = msg->ExtId;
	}

	txObj.bF.ctrl.BRS = 1;
	txObj.bF.ctrl.FDF = 0;

	if(msg->RTR == CAN_RTR_DATA) {
		txObj.bF.ctrl.RTR = 0;
	} else {
		txObj.bF.ctrl.RTR = 1;
	}

	txObj.bF.ctrl.ESI = 0;

	status = HAL_BUSY;

	while(status != HAL_OK) {
		mutex_lock(can_info->hcan_mutex);

		if(DRV_CANFDSPI_TransmitChannelEventGet(can_info->can_id, can_info->can_config->tx_fifo, &event) == 0) {
			if(event & CAN_TX_FIFO_NOT_FULL_EVENT) {
				// Load message and transmit
				uint8_t n = DRV_CANFDSPI_DlcToDataBytes(txObj.bF.ctrl.DLC);

				if(DRV_CANFDSPI_TransmitChannelLoad(can_info->can_id, can_info->can_config->tx_fifo, &txObj, msg->Data, n, true) == 0) {
					status = HAL_OK;
				}
			}

		}

		mutex_unlock(can_info->hcan_mutex);

		if(ticks_duration(osKernelSysTick(), - stamp) >= timeout) {
			break;
		}

		if(status != HAL_OK) {
			osDelay(5);
		}
	}

	if(status == HAL_OK) {
		ret = 0;
	}

	return ret;
}


static int _can_rx_data(void *_can_info, uint32_t timeout)
{
	int ret = -1;
	can_info_t *can_info = (can_info_t *)_can_info;

	if(can_info->rx_msg_r == can_info->rx_msg_w) {//没有数据
		CAN_RX_MSGOBJ rxObj;
		CAN_RX_FIFO_EVENT event = CAN_RX_FIFO_NO_EVENT;
		ret = signal_wait(can_info->rx_msg_q, NULL, timeout);

		if(ret == 0) {
			ret = -1;
			mutex_lock(can_info->hcan_mutex);

			if(DRV_CANFDSPI_ReceiveChannelEventGet(can_info->can_id, can_info->can_config->filter_fifo, &event) != 0) {
				event = CAN_RX_FIFO_NO_EVENT;
			}

			while((event & CAN_RX_FIFO_NOT_EMPTY_EVENT) != 0) {
				can_rx_msg_t *rx_msg;
				event = CAN_RX_FIFO_NO_EVENT;

				rx_msg = &can_info->rx_msg[can_info->rx_msg_w];
				can_info->rx_msg_w++;

				if(can_info->rx_msg_w >= CAN_RX_MSG_BUFFER_SIZE) {
					can_info->rx_msg_w = 0;
				}

				// Get message
				if(DRV_CANFDSPI_ReceiveMessageGet(can_info->can_id, can_info->can_config->filter_fifo, &rxObj, rx_msg->Data, 8) == 0) {
					if(rxObj.bF.ctrl.IDE == 0) {
						rx_msg->IDE = CAN_ID_STD;
						rx_msg->StdId = rxObj.bF.id.SID;
					} else {

						rx_msg->IDE = CAN_ID_EXT;
						rx_msg->ExtId = (rxObj.bF.id.SID << 18) | rxObj.bF.id.EID;
					}

					if(rxObj.bF.ctrl.RTR == 0) {
						rx_msg->RTR = CAN_RTR_DATA;
					} else {
						rx_msg->RTR = CAN_RTR_REMOTE;
					}

					rx_msg->DLC = rxObj.bF.ctrl.DLC;
					ret = 0;
				}

				if(DRV_CANFDSPI_ReceiveChannelEventGet(can_info->can_id, can_info->can_config->filter_fifo, &event) != 0) {
					event = CAN_RX_FIFO_NO_EVENT;
				}
			}

			mutex_unlock(can_info->hcan_mutex);
		}

	} else {//有数据
		ret = 0;
	}

	if(ret == 0) {
		can_info->rx_msg_pos = can_info->rx_msg_r;

		if((can_info->rx_msg_r + 1) >= CAN_RX_MSG_BUFFER_SIZE) {
			can_info->rx_msg_r = 0;
		} else {
			can_info->rx_msg_r++;
		}
	}

	return ret;
}

can_ops_t can_ops_spi_can = {
	.type = CAN_TYPE_SPI_CAN,
	.can_init = _can_init,
	.can_tx_data = _can_tx_data,
	.can_rx_data = _can_rx_data,
};
