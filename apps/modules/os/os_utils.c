

/*================================================================
 *
 *
 *   文件名称：os_utils.c
 *   创 建 者：肖飞
 *   创建日期：2019年11月13日 星期三 11时13分17秒
 *   修改日期：2021年01月29日 星期五 23时04分19秒
 *   描    述：
 *
 *================================================================*/
#include "os_utils.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cmsis_os.h"
#include "list_utils.h"
#include "FreeRTOSConfig.h"

typedef struct {
	size_t size;
	struct list_head list;
} mem_node_info_t;

typedef struct {
	uint8_t init;
	osMutexId os_utils_mutex;
	size_t size;
	size_t count;
	size_t max_size;
	struct list_head mem_info_list;
} mem_info_t;

static mem_info_t mem_info = {
	.init = 0,
	.os_utils_mutex = NULL,
};

void app_panic(void)
{
	while(1);
}

osMutexId mutex_create(void)
{
	osMutexId mutex = NULL;
	osMutexDef(mutex);

	mutex = osMutexCreate(osMutex(mutex));

	return mutex;
}

void mutex_delete(osMutexId mutex)
{
	osStatus os_status;

	if(mutex == NULL) {
		app_panic();
	}

	os_status = osMutexDelete(mutex);

	if(osOK != os_status) {
		app_panic();
	}
}

void mutex_lock(osMutexId mutex)
{
	osStatus os_status;

	if(mutex == NULL) {
		app_panic();
	}

	//os_status = osMutexWait(mutex, osWaitForever);
	os_status = osMutexWait(mutex, 1000);

	if(os_status != osOK) {
		app_panic();
	}
}

void mutex_unlock(osMutexId mutex)
{
	osStatus os_status;

	if(mutex == NULL) {
		app_panic();
	}

	os_status = osMutexRelease(mutex);

	if(os_status != osOK) {
		app_panic();
	}
}

osMessageQId signal_create(void)
{
	osMessageQId signal = NULL;
	osMessageQDef(signal, 1, uint16_t);

	signal = osMessageCreate(osMessageQ(signal), NULL);

	return signal;
}

void signal_delete(osMessageQId signal)
{
	osStatus os_status;

	if(signal == NULL) {
		app_panic();
	}

	os_status = osMessageDelete(signal);

	if(osOK != os_status) {
		app_panic();
	}
}

int signal_wait(osMessageQId signal, uint32_t timeout)
{
	int ret = -1;

	if(signal == NULL) {
		app_panic();
	}

	osEvent event = osMessageGet(signal, timeout);

	if(event.status == osEventMessage) {
		ret = 0;
	}

	return ret;
}

int signal_send(osMessageQId signal)
{
	int ret = -1;
	osStatus status;

	if(signal == NULL) {
		app_panic();
	}

	status = osMessagePut(signal, 0, 0);

	if(status != osOK) {
		ret = 0;
	}

	return ret;
}

static int init_mem_info(void)
{
	int ret = -1;

	if(mem_info.init == 1) {
		ret = 0;
		return ret;
	}

	mem_info.os_utils_mutex = mutex_create();

	if(mem_info.os_utils_mutex == NULL) {
		app_panic();
	}

	mem_info.size = 0;
	mem_info.count = 0;
	mem_info.max_size = 0;
	INIT_LIST_HEAD(&mem_info.mem_info_list);

	mem_info.init = 1;
	ret = 0;

	return ret;
}

static void *xmalloc(size_t size)
{
	mem_node_info_t *mem_node_info;

	__disable_irq();
	init_mem_info();
	__enable_irq();

	mutex_lock(mem_info.os_utils_mutex);

	mem_node_info = (mem_node_info_t *)pvPortMalloc(sizeof(mem_node_info_t) + size);

	if(mem_node_info != NULL) {
		mem_info.size += size;
		mem_info.count += 1;

		if(mem_info.size > mem_info.max_size) {
			mem_info.max_size = mem_info.size;
		}

		mem_node_info->size = size;
		list_add_tail(&mem_node_info->list, &mem_info.mem_info_list);
	}

	mutex_unlock(mem_info.os_utils_mutex);

	return (mem_node_info != NULL) ? (mem_node_info + 1) : NULL;
}

static void xfree(void *p)
{
	__disable_irq();
	init_mem_info();
	__enable_irq();

	mutex_lock(mem_info.os_utils_mutex);

	if(p != NULL) {
		mem_node_info_t *mem_node_info = (mem_node_info_t *)p;

		mem_node_info--;

		mem_info.size -= mem_node_info->size;
		mem_info.count -= 1;

		list_del(&mem_node_info->list);
		vPortFree(mem_node_info);
	}

	mutex_unlock(mem_info.os_utils_mutex);
}

void get_mem_info(size_t *size, size_t *count, size_t *max_size)
{
	mem_node_info_t *mem_node_info;
	struct list_head *head;

	*size = 0;
	*count = 0;
	*max_size = 0;

	__disable_irq();
	init_mem_info();
	__enable_irq();

	mutex_lock(mem_info.os_utils_mutex);

	*size = mem_info.size;
	*count = mem_info.count;
	*max_size = mem_info.max_size;

	head = &mem_info.mem_info_list;

	list_for_each_entry(mem_node_info, head, mem_node_info_t, list) {
	}

	mutex_unlock(mem_info.os_utils_mutex);
}

uint32_t get_total_heap_size(void)
{
	return (uint32_t)configTOTAL_HEAP_SIZE;
}

void *os_alloc(size_t size)
{
	void *p;

	p = xmalloc(size);

	return p;
}

void os_free(void *p)
{
	xfree(p);
}

unsigned char mem_is_set(char *values, size_t size, char value)
{
	unsigned char ret = 1;
	int i;

	for(i = 0; i < size; i++) {
		if(values[i] != value) {
			ret = 0;
			break;
		}
	}

	return ret;
}

unsigned int str_hash(const char *s)
{
	unsigned int hash = 0;
	const char *p = NULL;

	p = s;

	while(*p != 0) {
		hash = (31 * hash) + tolower(*p);
		p++;
	}

	return hash;
}

unsigned char calc_crc8(const void *data, size_t size)
{
	unsigned char crc = 0;
	unsigned char *p = (unsigned char *)data;

	while(size > 0) {
		crc += *p;

		p++;
		size--;
	}

	return crc;
}
