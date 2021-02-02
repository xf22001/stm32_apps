

/*================================================================
 *
 *
 *   文件名称：test_object_class.c
 *   创 建 者：肖飞
 *   创建日期：2021年02月02日 星期二 12时13分20秒
 *   修改日期：2021年02月02日 星期二 12时56分59秒
 *   描    述：
 *
 *================================================================*/
#include "test_object_class.h"
#include "object_class.h"
#include "log.h"

typedef struct {
	uint32_t id;
} test_object_t;

static void test_object_free(test_object_t *test_object)
{
	if(test_object == NULL) {
		return;
	}

	os_free(test_object);
}

test_object_t *test_object_alloc(uint32_t id)
{
	test_object_t *test_object = NULL;

	test_object = (test_object_t *)os_alloc(sizeof(test_object_t));

	if(test_object == NULL) {
		return test_object;
	}

	test_object->id = id;

	return test_object;
}


static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	test_object_t *test_object = (test_object_t *)o;
	uint32_t id = (uint32_t)ctx;

	if(test_object->id == id) {
		ret = 0;
	}

	return ret;
}

static int object_filter_no_filter(void *o, void *ctx)
{
	int ret = 0;

	return ret;
}

void test_object_class(void)
{
	object_class_t *object_class = NULL;
	size_t size;
	test_object_t **objects;
	int i;
	int ret;

	object_class = object_class_alloc();

	if(object_class == NULL) {
		debug("object_class_alloc failed!\n");
		return;
	}

	object_class_get_or_alloc_object(object_class, object_filter, (void *)1, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)1, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)6, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)1, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)1, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)1, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)5, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)2, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)4, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)2, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)3, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)3, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)2, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);
	object_class_get_or_alloc_object(object_class, object_filter, (void *)1, (object_alloc_t)test_object_alloc, (object_free_t)test_object_free);

	size = object_class_size(object_class);
	debug("object_class_size:%d\n", size);

	for(i = 0; i < 10; i++) {
		test_object_t *o = object_class_get_object(object_class, object_filter, (void *)i);

		if(o != NULL ) {
			debug("get object id %d success(%d)!\n", i, o->id);
		} else {
			debug("get object id %d failed!\n", i);
		}
	}

	objects = (test_object_t **)os_alloc(sizeof(test_object_t *) * size);

	if(objects != NULL) {
		ret = object_class_get_objects(object_class, object_filter_no_filter, NULL, (void **)objects, &size);
		debug("object_class_get_objects size:%d, ret:%d\n", size, ret);

		for(i = 0; i < size; i++) {
			uint32_t id = objects[i]->id;
			ret = object_class_remove_object(object_class, objects[i]);
			debug("remove object id:%d, ret:%d\n", id, ret);
		}

		os_free(objects);
	}

	size = object_class_size(object_class);
	debug("object_class_size:%d\n", size);

	object_class_free(object_class);
}
