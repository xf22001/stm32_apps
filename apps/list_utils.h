

/*================================================================
 *   
 *   
 *   文件名称：list_utils.h
 *   创 建 者：肖飞
 *   创建日期：2020年03月19日 星期四 13时41分50秒
 *   修改日期：2020年03月19日 星期四 13时42分22秒
 *   描    述：
 *
 *================================================================*/
#ifndef _LIST_UTILS_H
#define _LIST_UTILS_H
#ifdef __cplusplus
extern "C"
{
#endif

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new_list_item entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new_list_item, struct list_head *prev, struct list_head *next)
{
	next->prev = new_list_item;
	new_list_item->next = next;
	new_list_item->prev = prev;
	prev->next = new_list_item;
}

/**
 * list_add - add a new_list_item entry
 * @new_list_item: new_list_item entry to be added
 * @head: list head to add it after
 *
 * Insert a new_list_item entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new_list_item, struct list_head *head)
{
	__list_add(new_list_item, head, head->next);
}


/**
 * list_add_tail - add a new_list_item entry
 * @new_list_item: new_list_item entry to be added
 * @head: list head to add it before
 *
 * Insert a new_list_item entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new_list_item, struct list_head *head)
{
	__list_add(new_list_item, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void __list_del_entry(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline void list_del(struct list_head *entry)
{
	__list_del_entry(entry);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void list_del_init(struct list_head *entry)
{
	__list_del_entry(entry);
	INIT_LIST_HEAD(entry);
}

/**
 * list_replace - replace old entry by new_list_item one
 * @old : the element to be replaced
 * @new_list_item : the new_list_item element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void list_replace(struct list_head *old, struct list_head *new_list_item)
{
	new_list_item->next = old->next;
	new_list_item->next->prev = new_list_item;
	new_list_item->prev = old->prev;
	new_list_item->prev->next = new_list_item;
}

static inline void list_replace_init(struct list_head *old, struct list_head *new_list_item)
{
	list_replace(old, new_list_item);
	INIT_LIST_HEAD(old);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void list_move(struct list_head *list, struct list_head *head)
{
	__list_del_entry(list);
	list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void list_move_tail(struct list_head *list, struct list_head *head)
{
	__list_del_entry(list);
	list_add_tail(list, head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int list_is_last(const struct list_head *list, const struct list_head *head)
{
	return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

#ifndef offsetof
#define offsetof(typ, memb) ((unsigned long)((char *)&(((typ *)0)->memb)))
#endif

#define container_of(ptr, type, member) ((type *)((char *)ptr - offsetof(type, member)))

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_head within the struct.
 */
#define list_next_entry(pos, type, member) \
	list_entry((pos)->member.next, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

/**
 * list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)


/** list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 */
#define list_for_each_entry(pos, head, type, member) \
	for (pos = list_first_entry(head, type, member); \
	     &pos->member != (head); \
	     pos = list_next_entry(pos, type, member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 */
#define list_for_each_entry_reverse(pos, head, type, member) \
	for (pos = list_last_entry(head, type, member); \
	     &pos->member != (head); \
	     pos = list_prev_entry(pos, member))

#ifdef __cplusplus
}
#endif
#endif //_LIST_UTILS_H
