

/*================================================================
 *
 *
 *   文件名称：os_utils.c
 *   创 建 者：肖飞
 *   创建日期：2019年11月13日 星期三 11时13分17秒
 *   修改日期：2021年05月24日 星期一 13时56分48秒
 *   描    述：
 *
 *================================================================*/
#include "os_utils.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "list_utils.h"

#include "duty_cycle_pattern.h"

typedef struct {
	size_t size;
	struct list_head list;
} mem_node_info_t;

typedef struct {
	uint8_t init;
	os_mutex_t mutex;
	size_t size;
	size_t count;
	size_t max_size;
	struct list_head list;
} mem_info_t;

static mem_info_t mem_info = {
	.init = 0,
	.mutex = NULL,
};

void app_panic(void)
{
	set_work_led_fault_state(1);

	while(1);
}

os_mutex_t mutex_create(void)
{
	os_mutex_t mutex = NULL;
	osMutexDef(mutex);

	mutex = osMutexCreate(osMutex(mutex));

	return mutex;
}

void mutex_delete(os_mutex_t mutex)
{
	osStatus os_status;

	OS_ASSERT(mutex != NULL);
	os_status = osMutexDelete(mutex);
	OS_ASSERT(os_status == osOK);
}

void mutex_lock(os_mutex_t mutex)
{
	osStatus os_status;

	OS_ASSERT(mutex != NULL);
	//os_status = osMutexWait(mutex, osWaitForever);
	os_status = osMutexWait(mutex, 3 * 60 * 1000);
	OS_ASSERT(os_status == osOK);
}

void mutex_unlock(os_mutex_t mutex)
{
	osStatus os_status;

	OS_ASSERT(mutex != NULL);
	os_status = osMutexRelease(mutex);
	OS_ASSERT(os_status == osOK);
}

os_signal_t signal_create(size_t size)
{
	os_signal_t signal = NULL;
	osMessageQDef_t msg_queue = {0};

	msg_queue.queue_sz = size;
	msg_queue.item_sz = sizeof(uint32_t);

	signal = osMessageCreate(&msg_queue, NULL);

	return signal;
}

void signal_delete(os_signal_t signal)
{
	osStatus os_status;

	OS_ASSERT(signal != NULL);
	os_status = osMessageDelete(signal);
	OS_ASSERT(os_status == osOK);
}

int signal_wait(os_signal_t signal, uint32_t *pvalue, uint32_t timeout)
{
	int ret = -1;
	osEvent event;

	OS_ASSERT(signal != NULL);
	event = osMessageGet(signal, timeout);

	if(event.status == osEventMessage) {
		if(pvalue != NULL) {
			*pvalue = event.value.v;
		}

		ret = 0;
	}

	return ret;
}

int signal_send(os_signal_t signal, uint32_t value, uint32_t timeout)
{
	int ret = -1;
	osStatus os_status;

	OS_ASSERT(signal != NULL);
	os_status = osMessagePut(signal, value, timeout);

	if(os_status == osOK) {
		ret = 0;
	}

	return ret;
}

os_sem_t sem_create(int32_t value)
{
	os_sem_t sem = NULL;
	osSemaphoreDef(sem);

	sem = osSemaphoreCreate(osSemaphore(sem), value);

	return sem;
}

void sem_delete(os_sem_t sem)
{
	osStatus os_status;

	OS_ASSERT(sem != NULL);
	os_status = osSemaphoreDelete(sem);
	OS_ASSERT(os_status == osOK);
}

int sem_take(os_sem_t sem, uint32_t timeout)
{
	int ret = -1;
	osStatus os_status;

	OS_ASSERT(sem != NULL);
	os_status = osSemaphoreWait(sem, timeout);

	if(os_status == osOK) {
		ret = 0;
	}

	return ret;
}

int sem_release(os_sem_t sem)
{
	int ret = -1;
	osStatus os_status;

	OS_ASSERT(sem != NULL);
	os_status = osSemaphoreRelease(sem);
	OS_ASSERT(os_status == osOK);

	return ret;
}

static uint32_t os_critical_state = 0;

uint32_t get_os_critical_state(void)
{
	return os_critical_state;
}

void os_enter_critical(void)
{
	if(__get_IPSR() == 0) {
		taskENTER_CRITICAL();
		os_critical_state++;
	}
}

void os_leave_critical(void)
{
	if(__get_IPSR() == 0) {
		os_critical_state--;
		taskEXIT_CRITICAL();
	}
}

__weak void *port_malloc(size_t size)
{
	app_panic();
	return NULL;
}

__weak void port_free(void *p)
{
	app_panic();
}

__weak uint32_t get_total_heap_size(void)
{
	app_panic();
	return 0;
}

int init_mem_info(void)
{
	int ret = -1;

	if(mem_info.init == 1) {
		ret = 0;
		return ret;
	}

	mem_info.mutex = mutex_create();
	OS_ASSERT(mem_info.mutex != NULL);
	mem_info.size = 0;
	mem_info.count = 0;
	mem_info.max_size = 0;
	INIT_LIST_HEAD(&mem_info.list);

	mem_info.init = 1;
	ret = 0;

	return ret;
}

static void *xmalloc(size_t size)
{
	mem_node_info_t *mem_node_info;

	mutex_lock(mem_info.mutex);

	mem_node_info = (mem_node_info_t *)port_malloc(sizeof(mem_node_info_t) + size);

	if(mem_node_info != NULL) {
		mem_info.size += size;
		mem_info.count += 1;

		if(mem_info.size > mem_info.max_size) {
			mem_info.max_size = mem_info.size;
		}

		mem_node_info->size = size;
		list_add_tail(&mem_node_info->list, &mem_info.list);
	}

	mutex_unlock(mem_info.mutex);

	return (mem_node_info != NULL) ? (mem_node_info + 1) : NULL;
}

static void xfree(void *p)
{
	mutex_lock(mem_info.mutex);

	if(p != NULL) {
		mem_node_info_t *mem_node_info = (mem_node_info_t *)p;

		mem_node_info--;

		mem_info.size -= mem_node_info->size;
		mem_info.count -= 1;

		list_del(&mem_node_info->list);
		port_free(mem_node_info);
	}

	mutex_unlock(mem_info.mutex);
}

void get_mem_info(size_t *size, size_t *count, size_t *max_size)
{
	mem_node_info_t *mem_node_info;
	struct list_head *head;

	*size = 0;
	*count = 0;
	*max_size = 0;

	mutex_lock(mem_info.mutex);

	*size = mem_info.size;
	*count = mem_info.count;
	*max_size = mem_info.max_size;

	head = &mem_info.list;

	list_for_each_entry(mem_node_info, head, mem_node_info_t, list) {
	}

	mutex_unlock(mem_info.mutex);
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

void *os_realloc(void *p, size_t size)
{
	void *old = p;

	if(size != 0) {
		p = xmalloc(size);
	} else {
		xfree(p);
		old = NULL;
		p = NULL;
	}

	if(p != NULL) {
		if(old != NULL) {
			mem_node_info_t *mem_node_info = (mem_node_info_t *)old;
			mem_node_info -= 1;

			if(size > mem_node_info->size) {
				size = mem_node_info->size;
			}

			memcpy(p, old, size);
			xfree(old);
		}
	}

	return p;
}

void *os_calloc(size_t n, size_t size)
{
	void *p = xmalloc(n * size);

	if(p != NULL) {
		memset(p, 0, n * size);
	}

	return p;
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

uint8_t sum_crc8(const void *data, size_t size)
{
	uint8_t crc = 0;
	uint8_t *p = (unsigned char *)data;
	int i;

	for(i = 0; i < size; i++) {
		crc += p[i];
	}

	return crc;
}

uint16_t sum_crc16(const void *data, size_t size)
{
	uint16_t crc = 0;
	uint8_t *p = (unsigned char *)data;
	int i;

	for(i = 0; i < size; i++) {
		crc += p[i];
	}

	return crc;
}

uint32_t sum_crc32(const void *data, size_t size)
{
	uint32_t crc = 0;
	uint8_t *p = (unsigned char *)data;
	int i;

	for(i = 0; i < size; i++) {
		crc += p[i];
	}

	return crc;
}

uint32_t ticks_duration(uint32_t a, uint32_t b)
{
	uint32_t duration_1 = a - b;
	uint32_t duration_2 = b - a;

	return (duration_1 <= duration_2) ? duration_1 : duration_2;
}
