

/*================================================================
 *   
 *   
 *   文件名称：websocket.h
 *   创 建 者：肖飞
 *   创建日期：2021年07月09日 星期五 15时45分05秒
 *   修改日期：2021年07月10日 星期六 13时47分10秒
 *   描    述：
 *
 *================================================================*/
#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

typedef enum {
	WS_OPCODE_CONTINUE = 0x00,
	WS_OPCODE_TXT = 0x01,
	WS_OPCODE_BIN = 0x02,
	WS_OPCODE_RSV1 = 0x07,
	WS_OPCODE_DISCONN = 0x08,
	WS_OPCODE_PING = 0x09,
	WS_OPCODE_PONG = 0x0a,
	WS_OPCODE_NONE = 0x0b,
	WS_OPCODE_RSV2 = 0x0f,
} ws_opcode_t;

char *get_ws_opcode_des(ws_opcode_t opcode);
int ws_build_key(char *in, size_t in_len, uint8_t do_random, char *key, size_t *size);
int ws_build_header(char *header, size_t size, char *host, char *port, char *path, char *key, char *content);
int ws_match_response_header(char *header, char *match_str);
int ws_encode(uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len, uint8_t fin, ws_opcode_t opcode, uint8_t mask);
int ws_decode(uint8_t *in, size_t in_len, uint8_t **out, size_t *out_len, uint8_t *fin, ws_opcode_t *opcode);

#endif //_WEBSOCKET_H
