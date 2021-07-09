

/*================================================================
 *
 *
 *   文件名称：websocket.c
 *   创 建 者：肖飞
 *   创建日期：2021年07月09日 星期五 13时13分01秒
 *   修改日期：2021年07月10日 星期六 00时00分19秒
 *   描    述：
 *
 *================================================================*/
#include "websocket.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "os_utils.h"
#include "mbedtls/base64.h"
#include "log.h"

//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-------+-+-------------+-------------------------------+
// |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
// |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
// |N|V|V|V|       |S|             |   (if payload len==126/127)   |
// | |1|2|3|       |K|             |                               |
// +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
// |     Extended payload length continued, if payload len == 127  |
// + - - - - - - - - - - - - - - - +-------------------------------+
// |                               |Masking-key, if MASK set to 1  |
// +-------------------------------+-------------------------------+
// | Masking-key (continued)       |          Payload Data         |
// +-------------------------------- - - - - - - - - - - - - - - - +
// :                     Payload Data continued ...                :
// + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
// |                     Payload Data continued ...                |
// +---------------------------------------------------------------+
//FIN：1 bit
//表示这是不是消息的最后一帧。第一帧也有可能是最后一帧。 %x0： 还有后续帧 %x1：最后一帧
//RSV1、RSV2、RSV3：1 bit
//扩展字段，除非一个扩展经过协商赋予了非零值的某种含义，否则必须为0
//opcode：4 bit
//解释 payload data 的类型，如果收到识别不了的opcode，直接断开。分类值如下： %x0：连续的帧 %x1：text帧 %x2：binary帧 %x3 - 7：为非控制帧而预留的 %x8：关闭握手帧 %x9：ping帧 %xA：pong帧 %xB - F：为非控制帧而预留的
//MASK：1 bit
//标识 Payload data 是否经过掩码处理，如果是 1，Masking-key域的数据即为掩码密钥，用于解码Payload data。协议规定客户端数据需要进行掩码处理，所以此位为1
//Payload len：7 bit | 7+16 bit | 7+64 bit
//表示了 “有效负荷数据 Payload data”，以字节为单位： - 如果是 0~125，那么就直接表示了 payload 长度 - 如果是 126，那么 先存储 0x7E（=126）接下来的两个字节表示的 16位无符号整型数的值就是 payload 长度 - 如果是 127，那么 先存储 0x7E（=126）接下来的八个字节表示的 64位无符号整型数的值就是 payload 长度
//Masking-key：0 | 4 bytes 掩码密钥，所有从客户端发送到服务端的帧都包含一个 32bits 的掩码（如果mask被设置成1），否则为0。一旦掩码被设置，所有接收到的 payload data 都必须与该值以一种算法做异或运算来获取真实值。
//ws协议中，数据掩码的作用是增强协议的安全性。但数据掩码并不是为了保护数据本身，因为算法本身是公开的，运算也不复杂。除了加密通道本身，似乎没有太多有效的保护通信安全的办法，那么为什么还要引入掩码计算呢，除了增加计算机器的运算量外似乎并没有太多的收益（这也是不少同学疑惑的点）
//答案还是两个字：安全。但并不是为了防止数据泄密，而是为了防止早期版本的协议中存在的代理缓存污染攻击（proxy cache poisoning attacks）等问题
//Payload data：(x+y) bytes
//它是 Extension data 和 Application data 数据的总和，但是一般扩展数据为空。
//Extension data：x bytes
//除非扩展被定义，否则就是0
//Application data：y bytes
//占据 Extension data 后面的所有空间

#pragma pack(push, 1)

typedef struct {
	uint8_t fin : 1;//0:中间帧 1:最后一帧
	uint8_t rsv : 3;
	uint8_t opcode : 4;//0x00:连续帧 0x02:文本帧 0x03:二进制值帧 0x07:为非控制帧而预留的 0x08:关闭握手帧 0x09:ping帧 0x0a:pong帧 0x0b:- 0x0f:为非控制帧而预留的

	uint8_t mask : 1;//标识 Payload data 是否经过掩码处理，如果是 1，Masking-key域的数据即为掩码密钥，用于解码Payload data。协议规定客户端数据需要进行掩码处理
	uint8_t payload_len : 7;//7 bit | 7+16 bit | 7+64 bit
	uint8_t payload_len_ext[0];
} ws_data_header_t;

typedef struct {
	uint8_t mask[4];
} ws_mask_data_t;

#pragma pack(pop)

int ws_build_key(char *in, size_t in_len, uint8_t do_random, char *key, size_t *size)
{
	int ret = -1;
	size_t capicity = *size;
	size_t key_size = in_len * 4 / 3 + 4;
	int i;

	if(capicity < key_size) {
		return ret;
	}

	if(do_random != 0) {
		for(i = 0; i < in_len; i++) {
			in[i] = (rand() % (127 - 32)) + 32;
		}
	}

	ret = mbedtls_base64_encode((unsigned char *)key, capicity, size, (unsigned char *)in, in_len);

	return ret;
}

int ws_build_header(char *header, size_t size, char *host, char *port, char *path, char *key, char *content)
{
	return snprintf(header,
	                size,
	                "GET /%s HTTP/1.1\r\n"
	                "Connection: Upgrade\r\n"
	                "Host: %s:%s\r\n"
	                "Sec-WebSocket-Key: %s\r\n"
	                "Sec-WebSocket-Version: 13\r\n"
	                "Upgrade: websocket\r\n"
	                "%s"
	                "\r\n",
	                path,
	                host,
	                port,
	                key,
	                content);
}

int ws_match_response_header(char *header, char *match_str)
{
	int ret = -1;

	if(match_str == NULL) {
		match_str = "Sec-WebSocket-Accept:";
	}

	char *accept_str = strstr((const char *)header, (const char *)match_str);

	if(accept_str != NULL) {
		ret = 0;
	}

	return ret;
}

int ws_encode(uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len, uint8_t fin, ws_opcode_t opcode, uint8_t mask)
{
	int ret = -1;
	size_t capicity = *out_len;
	size_t len = sizeof(ws_data_header_t);
	ws_data_header_t *ws_data_header = (ws_data_header_t *)out;
	ws_mask_data_t *ws_mask_data = NULL;
	uint8_t *payload = NULL;

	if(capicity < len) {
		debug("");
		return ret;
	}

	ws_data_header->fin = fin;
	ws_data_header->opcode = opcode;
	ws_data_header->mask = mask;

	if(in_len <= 125) {
		len += 0;

		if(capicity < len) {
			debug("");
			return ret;
		}

		ws_data_header->payload_len = in_len;

		ws_mask_data = (ws_mask_data_t *)ws_data_header->payload_len_ext;
	} else if(in_len < 0xffff) {
		len += 2;

		if(capicity < len) {
			debug("");
			return ret;
		}

		ws_data_header->payload_len = 126;
		ws_data_header->payload_len_ext[0] = get_u8_h_from_u16(in_len);
		ws_data_header->payload_len_ext[1] = get_u8_l_from_u16(in_len);

		ws_mask_data = (ws_mask_data_t *)ws_data_header->payload_len_ext + 2;
	} else if(in_len < 0xffffffff) {
		len += 8;

		if(capicity < len) {
			debug("");
			return ret;
		}

		ws_data_header->payload_len = 127;
		ws_data_header->payload_len_ext[0] = get_u8_b3_from_u32(in_len);
		ws_data_header->payload_len_ext[1] = get_u8_b2_from_u32(in_len);
		ws_data_header->payload_len_ext[2] = get_u8_b1_from_u32(in_len);
		ws_data_header->payload_len_ext[3] = get_u8_b0_from_u32(in_len);
		ws_data_header->payload_len_ext[4] = 0;
		ws_data_header->payload_len_ext[5] = 0;
		ws_data_header->payload_len_ext[6] = 0;
		ws_data_header->payload_len_ext[7] = 0;
		ws_mask_data = (ws_mask_data_t *)ws_data_header->payload_len_ext + 8;
	}

	OS_ASSERT(ws_mask_data != NULL);

	if(mask != 0) {
		len += sizeof(ws_mask_data_t) + in_len;

		if(capicity < len) {
			debug("");
			return ret;
		}

		payload = (uint8_t *)(ws_mask_data + 1);
		int i;

		for(i = 0; i < in_len; i++) {
			payload[i] = in[i] ^ ws_mask_data->mask[i % 4];
		}
	} else {
		len += in_len;

		payload = (uint8_t *)ws_mask_data;
		memcpy(payload, in, in_len);
	}

	*out_len = len;

	ret = 0;

	return ret;
}

int ws_decode(uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len, uint8_t *fin, ws_opcode_t *opcode)
{
	int ret = -1;
	size_t capicity = in_len;
	size_t len = sizeof(ws_data_header_t);
	ws_data_header_t *ws_data_header = (ws_data_header_t *)in;
	ws_mask_data_t *ws_mask_data = NULL;
	uint8_t *payload = NULL;
	uint8_t mask;
	size_t payload_len;

	if(capicity < len) {
		debug("");
		return ret;
	}

	*fin = ws_data_header->fin;
	*opcode = ws_data_header->opcode;

	payload_len = ws_data_header->payload_len;
	mask = ws_data_header->mask;

	if(payload_len <= 125) {
		ws_mask_data = (ws_mask_data_t *)ws_data_header->payload_len_ext;
	} else if(payload_len == 126) {
		len += 2;

		if(capicity < len) {
			debug("");
			return ret;
		}

		payload_len = get_u16_from_u8_lh(ws_data_header->payload_len_ext[1],
		                                 ws_data_header->payload_len_ext[0]);

		ws_mask_data = (ws_mask_data_t *)ws_data_header->payload_len_ext + 2;
	} else if(payload_len == 127) {
		len += 8;

		if(capicity < len) {
			debug("");
			return ret;
		}

		payload_len = get_u32_from_u8_b0123(ws_data_header->payload_len_ext[3],
		                                    ws_data_header->payload_len_ext[2],
		                                    ws_data_header->payload_len_ext[1],
		                                    ws_data_header->payload_len_ext[0]);

		ws_mask_data = (ws_mask_data_t *)ws_data_header->payload_len_ext + 8;
	}

	OS_ASSERT(ws_mask_data != NULL);

	if(*out_len < payload_len) {
		debug("");
		return ret;
	}

	if(mask != 0) {
		int i;
		len += sizeof(ws_mask_data_t) + payload_len;

		if(capicity < len) {
			debug("");
			return ret;
		}

		payload = (uint8_t *)(ws_mask_data + 1);

		for(i = 0; i < in_len; i++) {
			out[i] = payload[i] ^ ws_mask_data->mask[i % 4];
		}
	} else {
		len += payload_len;

		if(capicity < len) {
			debug("");
			return ret;
		}

		payload = (uint8_t *)ws_mask_data;

		memcpy(out, payload, payload_len);
	}

	*out_len = payload_len;

	ret = 0;
	return ret;
}
