

/*================================================================
 *   Copyright (C) 2017年08月18日 肖飞 All rights reserved
 *
 *   文件名称：memory.c
 *   创 建 者：肖飞

 *   创建日期：2017年08月18日 星期五 09时23分55秒
 *   修改日期：2017年08月18日 星期五 10时31分54秒
 *   描    述：
 *
 *================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "list_utils.h"
#include "log.h"

#define myprintf(fmt, args...) debug(fmt, ## args)

typedef struct _mem_info {
	int initilized;
	unsigned long total_count;
	unsigned long total_size;
	unsigned long total_size_max;
	struct list_head head; //need to initilized
} mem_info_t;

typedef struct _node {
	struct list_head list;

	unsigned long long size;
	void *ptr;
} node_t;

mem_info_t mem_info = {0};

int init_mem_info()
{
	int ret = 0;
	mem_info.initilized = 1;
	mem_info.total_count = 0;
	mem_info.total_size = 0;
	mem_info.total_size_max = 0;
	INIT_LIST_HEAD(&mem_info.head);
	//myprintf("mem_info.head:%p\n", &mem_info.head);
	return ret;
}

int account_malloc(int size, void *ptr)
{
	int ret = 0;

	node_t *node = (node_t *)os_alloc(sizeof(node_t));

	if(node == NULL) {
		ret = -1;
		return ret;
	}

	if(mem_info.initilized == 0) {
		init_mem_info();
	}

	mem_info.total_count += 1;
	mem_info.total_size += size;

	node->ptr = ptr;
	node->size = size;
	list_add_tail(&node->list, &mem_info.head);

	if(mem_info.total_size > mem_info.total_size_max) {
		mem_info.total_size_max = mem_info.total_size;
		myprintf("[peak-value]:mem_info.total_count:%lu, mem_info.total_size:%lu\n", mem_info.total_count, mem_info.total_size);
	}

	return ret;
}

int account_free(void *ptr)
{
	int ret = 0;
	node_t *node = NULL;
	bool found = false;

	if(mem_info.initilized == 0) {
		init_mem_info();
	}

	if(list_empty(&mem_info.head)) {
		myprintf("meminfo:no meminfo!\n");
		ret = -1;
		return ret;
	}

	list_for_each_entry(node, &mem_info.head, node_t, list) {
		if(node->ptr == ptr) {
			found = true;
			break;
		}
	}

	if(!found) {
		myprintf("meminfo:not found mem block %p!\n", ptr);
		ret = -1;
		return ret;
	}

	mem_info.total_count -= 1;
	mem_info.total_size -= node->size;

	list_del(&node->list);
	os_free(node);

	return ret;
}

void *malloc_1(int size)
{
	int ret = 0;
	void *ptr = NULL;

	ptr = os_alloc(size);

	if(ptr == NULL) {
		return ptr;
	}

	ret = account_malloc(size, ptr);

	if(ret != 0) {
		os_free(ptr);
		ptr = NULL;
		return ptr;
	}

	return ptr;
}

void *calloc_1(size_t nmemb, size_t size)
{
	int ret = 0;
	void *ptr = NULL;

	ptr = os_alloc(nmemb * size);

	if(ptr == NULL) {
		return ptr;
	}

	ret = account_malloc(nmemb * size, ptr);

	if(ret != 0) {
		os_free(ptr);
		ptr = NULL;
		return ptr;
	}

	return ptr;
}

void free_1(void *ptr)
{
	int ret = 0;

	ret = account_free(ptr);

	if(ret != 0) {
		return;
	}

	os_free(ptr);
}

void *realloc_1(void *ptr, size_t size)
{
	int ret = 0;
	void *new_ptr = NULL;

	new_ptr = os_alloc(size);

	if(new_ptr == NULL) {
		return new_ptr;
	}

	os_free(ptr);
	ret = account_free(ptr);

	ret += account_malloc(size, new_ptr);

	if(ret != 0) {
		os_free(new_ptr);
		new_ptr = NULL;
		return new_ptr;
	}

	return new_ptr;
}

int p_mem_info()
{
	int ret = 0;

	//node_t *node = NULL;
	//unsigned long long total_count = 0;
	//unsigned long long total_size = 0;

	//if(mem_info.initilized == 0) {
	//	init_mem_info();
	//}

	//list_for_each_entry(node, &mem_info.head, node_t, list) {
	//	total_count += 1;
	//	total_size += node->size;
	//	myprintf("[%lu]:%p %lu\n", total_count, node->ptr, node->size);
	//}

	//myprintf("total_count:%lu, total_size:%lu\n", total_count, total_size);

	myprintf("mem_info.total_count:%lu, mem_info.total_size:%lu, [peak-value]:%lu\n", mem_info.total_count, mem_info.total_size, mem_info.total_size_max);
	return ret;
}
