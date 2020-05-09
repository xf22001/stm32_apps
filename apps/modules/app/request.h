

/*================================================================
 *   
 *   
 *   文件名称：request.h
 *   创 建 者：肖飞
 *   创建日期：2020年02月16日 星期日 11时03分12秒
 *   修改日期：2020年05月09日 星期六 16时49分16秒
 *   描    述：
 *
 *================================================================*/
#ifndef _REQUEST_PACKAGE_H
#define _REQUEST_PACKAGE_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "os_utils.h"

#define DEFAULT_REQUEST_MAGIC 0xa5a55a5a

#pragma pack(push, 1)

typedef struct {
	unsigned int magic;
	unsigned int total_size;
	unsigned int data_size;
	unsigned int data_offset;
	unsigned char crc;
} header_info_t;

typedef struct {
	unsigned int fn;
	unsigned int stage;
} payload_info_t;

typedef struct {
	header_info_t header;
	payload_info_t payload;
} request_t;

#pragma pack(pop)

typedef struct {
	unsigned int fn;
	unsigned int stage;
	const unsigned char *data;
	size_t size;
	size_t consumed;
	size_t max_request_size;
	request_t *request;
	size_t request_size;
} request_info_t;

void request_decode(char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size);
void request_encode(request_info_t *request_info);

#endif //_REQUEST_PACKAGE_H
