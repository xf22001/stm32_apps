

/*================================================================
 *   
 *   
 *   文件名称：request_monitor.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月09日 星期六 10时49分40秒
 *   修改日期：2020年05月12日 星期二 10时34分39秒
 *   描    述：
 *
 *================================================================*/
#include <string.h>

#include "net_client.h"
#include "request.h"
#include "task_probe_tool.h"
#include "main.h"

static int chunk_sendto(uint32_t fn, uint32_t stage, void *data, size_t size, char *send_buffer, size_t send_buffer_size)
{
	int ret = 0;
	request_info_t request_info;

	request_info.fn = (unsigned int)fn;
	request_info.stage = (unsigned int)stage;
	request_info.data = (const unsigned char *)data;
	request_info.size = size;
	request_info.consumed = 0;
	request_info.max_request_size = send_buffer_size;
	request_info.request = (request_t *)send_buffer;
	request_info.request_size = 0;

	while(request_info.size > request_info.consumed) {
		request_encode(&request_info);

		if(request_info.request_size != 0) {
			if(send_to_server((uint8_t *)send_buffer, request_info.request_size) != (int)request_info.request_size) {
				ret = -1;
				break;
			}
		}
	}

	if(ret != -1) {
		ret = size;
	}

	return ret;
}

static void request_set_lan_led_state(uint32_t state)
{
	if(state == 0) {
		HAL_GPIO_WritePin(led_lan_GPIO_Port, led_lan_Pin, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(led_lan_GPIO_Port, led_lan_Pin, GPIO_PIN_SET);
	}
}

static void request_init(void)
{
	udp_log_printf("%s:%s\n", __FILE__, __func__);
}

static void request_before_create_server_connect(void)
{
	udp_log_printf("%s:%s\n", __FILE__, __func__);
}

static void request_after_create_server_connect(void)
{
	udp_log_printf("%s:%s\n", __FILE__, __func__);
}

static void request_before_close_server_connect(void)
{
	udp_log_printf("%s:%s\n", __FILE__, __func__);
}

static void request_after_close_server_connect(void)
{
	udp_log_printf("%s:%s\n", __FILE__, __func__);
}

static void request_parse(char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size)
{
	request_decode((char *)buffer, size, max_request_size, prequest, request_size);
}

static void request_process(uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	udp_log_hexdump("request_process", (const char *)request, request_size);
}

static void request_periodic(uint8_t *send_buffer, uint16_t send_buffer_size)
{
	udp_log_printf("%s:%s\n", __FILE__, __func__);

	if(get_client_state() == CLIENT_CONNECTED) {
		chunk_sendto(1, 0, (void *)0x8000000, 128, (char *)send_buffer, send_buffer_size);
	}
}

request_callback_t request_callback_environment_monitor = {
	.name = "environment monitor",
	.set_lan_led_state = request_set_lan_led_state,
	.init = request_init,
	.before_connect = request_before_create_server_connect,
	.after_connect = request_after_create_server_connect,
	.before_close = request_before_close_server_connect,
	.after_close = request_after_close_server_connect,
	.parse = request_parse,
	.process = request_process,
	.periodic = request_periodic,
};
