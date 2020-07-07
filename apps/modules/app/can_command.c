

/*================================================================
 *
 *
 *   文件名称：can_command.c
 *   创 建 者：肖飞
 *   创建日期：2020年07月07日 星期二 08时29分24秒
 *   修改日期：2020年07月07日 星期二 15时01分24秒
 *   描    述：
 *
 *================================================================*/
#include "can_command.h"

#include <string.h>

#include "log.h"

#define add_des_case(e) \
	case e: { \
		des = #e; \
	} \
	break

char *get_can_com_state_des(can_com_com_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CAN_COM_STATE_IDLE);
			add_des_case(CAN_COM_STATE_REQUEST);
			add_des_case(CAN_COM_STATE_RESPONSE);
			add_des_case(CAN_COM_STATE_ERROR);

		default: {
		}
		break;
	}

	return des;
}

char *get_can_com_response_status_des(can_com_response_status_t status)
{
	char *des = "unknow";

	switch(status) {
			add_des_case(CAN_COM_RESPONSE_STATUS_WAIT);
			add_des_case(CAN_COM_RESPONSE_STATUS_DONE);

		default: {
		}
		break;
	}

	return des;
}

//准备请求数据 发
int can_com_prepare_request(can_com_cmd_ctx_t *can_com_cmd_ctx, can_com_cmd_common_t *can_com_cmd_common, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;

	uint8_t index = can_com_cmd_ctx->index;
	uint8_t *buffer = can_com_cmd_common->data;
	uint8_t buffer_size = sizeof(can_com_cmd_common->data);
	uint8_t sent = buffer_size * index;
	uint8_t send;

	can_com_cmd_common->index = index;

	if(sent >= data_size) {
		debug("sent:%d, data_size:%d\n", sent, data_size);
		return ret;
	}

	send = data_size - sent;

	if(send > buffer_size) {
		send = buffer_size;
	}

	//填状态数据
	memcpy(buffer, data + sent, send);

	//debug("sent %d/%d\n", sent + send, data_size);

	can_com_cmd_ctx->state = CAN_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

//请求数据后,处理响应响应 收
int can_com_process_response(can_com_cmd_ctx_t *can_com_cmd_ctx, can_com_cmd_response_t *can_com_cmd_response, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;
	uint8_t index = can_com_cmd_ctx->index;
	can_com_cmd_common_t *can_com_cmd_common = NULL;

	//检查index
	if(index != can_com_cmd_response->index) {
		debug("index %d, can_com_cmd_response->index %d\n", index, can_com_cmd_response->index);
		return ret;
	}

	index++;

	//检查数据尺寸
	if(index * sizeof(can_com_cmd_common->data) < data_size) {
		if(can_com_cmd_response->response_status == CAN_COM_RESPONSE_STATUS_WAIT) {
			can_com_cmd_ctx->index++;
			can_com_cmd_ctx->state = CAN_COM_STATE_REQUEST;//没发完,切换到准备请求数据 发
		} else {
			debug("can_com_cmd_response->response_status:%s\n", get_can_com_response_status_des(can_com_cmd_response->response_status));
			return ret;
		}
	} else {
		if(can_com_cmd_response->response_status == CAN_COM_RESPONSE_STATUS_WAIT) {//发完，返回空闲状态
			debug("can_com_cmd_response->response_status:%s\n", get_can_com_response_status_des(can_com_cmd_response->response_status));
			return ret;
		} else {
			can_com_cmd_ctx->state = CAN_COM_STATE_IDLE;
		}
	}

	ret = 0;

	return ret;
}

//处理请求后，准备响应数据 发
int can_com_prepare_response(can_com_cmd_ctx_t *can_com_cmd_ctx, can_com_cmd_response_t *can_com_cmd_response, uint8_t cmd, uint8_t data_size)
{
	int ret = -1;

	uint8_t index = can_com_cmd_ctx->index;
	can_com_cmd_common_t *can_com_cmd_common = NULL;

	//填index
	can_com_cmd_response->index = index;

	index++;

	//填状态
	if(index * sizeof(can_com_cmd_common->data) < data_size) {
		can_com_cmd_response->response_status = CAN_COM_RESPONSE_STATUS_WAIT;
	} else {
		can_com_cmd_response->response_status = CAN_COM_RESPONSE_STATUS_DONE;
	}

	can_com_cmd_ctx->state = CAN_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

//处理请求数据 收
int can_com_process_request(can_com_cmd_ctx_t *can_com_cmd_ctx, can_com_cmd_common_t *can_com_cmd_common, uint8_t cmd, uint8_t *data, uint8_t data_size)
{
	int ret = -1;
	uint8_t index = can_com_cmd_common->index;
	uint8_t *buffer = can_com_cmd_common->data;
	uint8_t buffer_size = sizeof(can_com_cmd_common->data);
	uint8_t received = buffer_size * index;
	uint8_t receive;

	if(received >= data_size) {
		debug("received:%d, data_size:%d\n", received, data_size);
		return ret;
	}

	receive = data_size - received;

	if(receive > buffer_size) {
		receive = buffer_size;
	}

	memcpy(data + received, buffer, receive);

	//debug("received %d/%d\n", received + receive, data_size);

	can_com_cmd_ctx->index = index;
	can_com_cmd_ctx->state = CAN_COM_STATE_REQUEST;

	ret = 0;

	return ret;
}

void can_com_set_connect_state(can_com_connect_state_t *can_com_connect_state, uint8_t state)
{
	can_com_connect_state->state[can_com_connect_state->index++] = state;

	if(can_com_connect_state->index >= CAN_COM_CONNECT_STATE_SIZE) {
		can_com_connect_state->index = 0;
	}
}

uint8_t can_com_get_connect_state(can_com_connect_state_t *can_com_connect_state)
{
	uint8_t count = 0;
	int i;

	for(i = 0; i < CAN_COM_CONNECT_STATE_SIZE; i++) {
		if(can_com_connect_state->state[i] != 0) {
			count++;
		}
	}

	return count;
}

