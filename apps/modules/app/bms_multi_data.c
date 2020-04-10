

/*================================================================
 *
 *
 *   文件名称：bms_multi_data.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月09日 星期四 13时04分55秒
 *   修改日期：2020年04月10日 星期五 16时48分27秒
 *   描    述：
 *
 *================================================================*/
#include "bms_multi_data.h"
#include <string.h>
//#define UDP_LOG
#include "task_probe_tool.h"

static int send_bms_multi_request_response(can_info_t *can_info, multi_packets_des_t *multi_packets_des, bms_data_settings_t *settings)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	bms_multi_request_response_t *data;

	tx_msg.ExtId = get_pdu_id(FN_MULTI_REQUEST_PRIORITY, FN_MULTI_REQUEST, settings->dst, settings->src);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bms_multi_request_response_t);

	data = (bms_multi_request_response_t *)tx_msg.Data;

	data->head.type = FN_MULTI_REQUEST_RESPONSE_TYPE;
	data->packets = multi_packets_des->bms_data_multi_packets;
	data->packet_index = multi_packets_des->bms_data_multi_next_index;
	data->reserved_0 = 0xff;
	data->reserved_1 = 0xff;
	data->reserved_2 = 0x00;
	data->fn = multi_packets_des->bms_data_multi_fn;
	data->reserved_3 = 0x00;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static int send_bms_multi_data_response(can_info_t *can_info, multi_packets_des_t *multi_packets_des, bms_data_settings_t *settings)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	bms_multi_data_response_t *data;

	tx_msg.ExtId = get_pdu_id(FN_MULTI_REQUEST_PRIORITY, FN_MULTI_REQUEST, settings->dst, settings->src);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bms_multi_data_response_t);

	data = (bms_multi_data_response_t *)tx_msg.Data;

	data->head.type = FN_MULTI_DATA_RESPONSE_TYPE;
	data->bytes = multi_packets_des->bms_data_multi_bytes;
	data->packets = multi_packets_des->bms_data_multi_packets;
	data->reserved_0 = 0xff;
	data->reserved_1 = 0x00;
	data->fn = multi_packets_des->bms_data_multi_fn;
	data->reserved_2 = 0x00;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static uint8_t *get_multi_packet_data_by_fn(bms_data_settings_t *settings, uint8_t fn)
{
	uint8_t *data = NULL;

	switch(fn) {
		case FN_BRM: {
			data = (uint8_t *)&settings->brm_data;
		}
		break;

		case FN_BCP: {
			data = (uint8_t *)&settings->bcp_data;
		}
		break;

		case FN_BCS: {
			data = (uint8_t *)&settings->bcs_data;
		}
		break;

		default:
			break;
	}

	return data;
}

int handle_multi_data_response(can_info_t *can_info, multi_packets_info_t *multi_packets_info, bms_data_settings_t *settings)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_MULTI_REQUEST: {
			bms_multi_request_head_t *request_head = (bms_multi_request_head_t *)rx_msg->Data;

			switch(request_head->type) {
				case FN_MULTI_REQUEST_TYPE: {//处理多包请求
					bms_multi_request_t *data = (bms_multi_request_t *)rx_msg->Data;
					multi_packets_des_t *multi_packets_des = &multi_packets_info->rx_des;

					multi_packets_des->bms_data_multi_bytes = data->bytes;
					multi_packets_des->bms_data_multi_packets = data->packets;
					multi_packets_des->bms_data_multi_next_index = 1;
					multi_packets_des->bms_data_multi_fn = data->fn;
					multi_packets_des->bms_data = get_multi_packet_data_by_fn(settings, data->fn);

					if(multi_packets_des->bms_data) {
						send_bms_multi_request_response(can_info, multi_packets_des, settings);
					}
				}
				break;

				case FN_MULTI_REQUEST_RESPONSE_TYPE: {//处理多包请求响应
					multi_packets_des_t *multi_packets_des = &multi_packets_info->tx_des;
					bms_multi_request_response_t *data = (bms_multi_request_response_t *)rx_msg->Data;

					if((data->packets == multi_packets_des->bms_data_multi_packets)
					   && (data->fn == multi_packets_des->bms_data_multi_fn)) {
						multi_packets_des->bms_data_multi_next_index = data->packet_index;
					}
				}
				break;

				case FN_MULTI_DATA_RESPONSE_TYPE: {//处理多包数据响应
					multi_packets_des_t *multi_packets_des = &multi_packets_info->tx_des;
					bms_multi_data_response_t *data = (bms_multi_data_response_t *)rx_msg->Data;

					udp_log_printf("%s process fn %02x data response\n",
					               (can_info->filter_fifo == 0x00) ? "hcan1" : "hcan2",
					               data->fn);
					//状态已经复位，下面代码无意义
					//if((data->bytes == multi_packets_des->bms_data_multi_bytes) && (data->packets == multi_packets_des->bms_data_multi_packets) && (data->fn == multi_packets_des->bms_data_multi_fn)) {
					//}
				}
				break;

				default:
					break;
			}

			ret = 0;
		}
		break;

		case FN_MULTI_DATA: {//处理多包数据
			bms_multi_data_t *data = (bms_multi_data_t *)rx_msg->Data;
			uint16_t received = (7 * (data->index - 1));
			uint16_t receive = 7;
			multi_packets_des_t *multi_packets_des = &multi_packets_info->rx_des;

			if(received < multi_packets_des->bms_data_multi_bytes) {
				if(receive > multi_packets_des->bms_data_multi_bytes - received) {
					receive = multi_packets_des->bms_data_multi_bytes - received;
				}

				if(data->index == multi_packets_des->bms_data_multi_next_index) {
					if(multi_packets_des->bms_data != NULL) {
						memcpy(multi_packets_des->bms_data + received, data->data, receive);
					}

					if(data->index < multi_packets_des->bms_data_multi_packets) {
						multi_packets_des->bms_data_multi_next_index++;
					} else if(data->index == multi_packets_des->bms_data_multi_packets) {
						udp_log_printf("%s send fn %02x data response\n",
						               (can_info->filter_fifo == 0x00) ? "hcan1" : "hcan2",
						               multi_packets_des->bms_data_multi_fn);
						send_bms_multi_data_response(can_info, multi_packets_des, settings);
					}
				}

			}

			ret = 0;
		}
		break;

		default:
			break;
	}

	return ret;
}

uint8_t is_bms_data_multi_received(can_info_t *can_info, multi_packets_info_t *multi_packets_info, bms_fn_t fn)
{
	can_rx_msg_t *rx_msg = can_get_msg(can_info);
	u_pdu_head_t head;
	uint8_t ret = 0;
	multi_packets_des_t *multi_packets_des = &multi_packets_info->rx_des;

	if(rx_msg == NULL) {
		return ret;
	}

	head.v = rx_msg->ExtId;

	if(head.pdu.pf == FN_MULTI_DATA) {
		bms_multi_data_t *data = (bms_multi_data_t *)rx_msg->Data;

		if((data->index == multi_packets_des->bms_data_multi_packets) && (data->index == multi_packets_des->bms_data_multi_next_index) && (multi_packets_des->bms_data_multi_fn == fn)) {
			ret = 1;
		}
	}

	return ret;
}

static int send_multi_request(can_info_t *can_info, multi_packets_des_t *multi_packets_des, bms_data_settings_t *settings)
{
	int ret = 0;
	can_tx_msg_t tx_msg;

	tx_msg.ExtId = get_pdu_id(FN_MULTI_REQUEST_PRIORITY, FN_MULTI_REQUEST, settings->dst, settings->src);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bms_multi_request_t);

	bms_multi_request_t *data = (bms_multi_request_t *)tx_msg.Data;
	data->head.type = FN_MULTI_REQUEST_TYPE;
	data->bytes = multi_packets_des->bms_data_multi_bytes;
	data->packets = multi_packets_des->bms_data_multi_packets;
	data->reserved_0 = 0xff;
	data->reserved_1 = 0x00;
	data->fn = multi_packets_des->bms_data_multi_fn;
	data->reserved_2 = 0x00;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static int send_multi_data(can_info_t *can_info, multi_packets_des_t *multi_packets_des, bms_data_settings_t *settings)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	int i = multi_packets_des->bms_data_multi_next_index;
	bms_multi_data_t *data = (bms_multi_data_t *)tx_msg.Data;

	if(i > multi_packets_des->bms_data_multi_packets) {
		return ret;
	}

	tx_msg.ExtId = get_pdu_id(FN_MULTI_DATA_PRIORITY, FN_MULTI_DATA, settings->dst, settings->src);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bms_multi_data_t);

	for(i; i <= multi_packets_des->bms_data_multi_packets; i = multi_packets_des->bms_data_multi_next_index) {
		uint8_t sent_bytes = (7 * (i - 1));
		uint8_t send_bytes = 7;

		if(send_bytes > (multi_packets_des->bms_data_multi_bytes - sent_bytes)) {
			send_bytes = multi_packets_des->bms_data_multi_bytes - sent_bytes;
		}

		data->index = i;

		memset(data->data, 0, sizeof(data->data));

		if(multi_packets_des->bms_data != NULL) {
			memcpy(data->data, multi_packets_des->bms_data + sent_bytes, send_bytes);
		}

		ret = can_tx_data(can_info, &tx_msg, 10);

		udp_log_printf("%s send fn:%02x, index:%d, ret:%d\n",
		               (can_info->filter_fifo == 0x00) ? "hcan1" : "hcan2",
		               multi_packets_des->bms_data_multi_fn,
		               multi_packets_des->bms_data_multi_next_index,
		               ret);

		if(ret != 0) {
			break;
		}

		multi_packets_des->bms_data_multi_next_index++;
	}


	return ret;
}

int send_multi_packets(can_info_t *can_info, multi_packets_info_t *multi_packets_info, bms_data_settings_t *settings, uint8_t *data, uint16_t fn, uint16_t bytes, uint16_t packets, uint32_t period)
{
	int ret = -1;
	multi_packets_des_t *multi_packets_des = &multi_packets_info->tx_des;
	uint32_t ticks = osKernelSysTick();

	if(multi_packets_des->bms_data_multi_fn != FN_INVALID) {
		if(ticks - multi_packets_des->start_stamp >= period) {
			multi_packets_des->bms_data_multi_fn = FN_INVALID;
			multi_packets_des->bms_data = NULL;
		}
	}

	if(multi_packets_des->bms_data_multi_fn == FN_INVALID) {
		multi_packets_des->bms_data_multi_bytes = bytes;
		multi_packets_des->bms_data_multi_packets = packets;
		multi_packets_des->bms_data_multi_next_index = 0;
		multi_packets_des->bms_data = data;
		multi_packets_des->start_stamp = ticks;
		multi_packets_des->bms_data_multi_fn = fn;

		send_multi_request(can_info, multi_packets_des, settings);
	} else if(multi_packets_des->bms_data_multi_fn == fn) {
		if((multi_packets_des->bms_data_multi_next_index >= 1) && (multi_packets_des->bms_data_multi_next_index <= multi_packets_des->bms_data_multi_packets)) {
			send_multi_data(can_info, multi_packets_des, settings);

			if(multi_packets_des->bms_data_multi_next_index > multi_packets_des->bms_data_multi_packets) {
				multi_packets_des->bms_data_multi_fn = FN_INVALID;
				ret = 0;
			}
		}
	}

	return ret;
}

