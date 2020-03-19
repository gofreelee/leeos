#ifndef LIST_H_
#define LIST_H_
#include "../stdint.h"
#define bool int
#define true 1
#define false 0
struct list_elem
{
    struct list_elem *prev;
    struct list_elem *next;
};

struct list
{
    struct list_elem head;
    struct list_elem tail;
};

//用于作回调函数
typedef bool(call_back_func)(struct list_elem *, int arg);

void list_init(struct list *);
void list_insert_before(struct list_elem *before, struct list_elem *elem);
void list_push(struct list *plist, struct list_elem *elem);
void list_iterate(struct list *plist);
void list_append(struct list *plist, struct list_elem *elem);
void list_remove(struct list_elem *elem);
struct list_elem *list_pop(struct list *plist);
bool list_empty(struct list *plist);
uint32_t list_len(struct list *plist);
struct list_elem *list_traveral(struct list *plist, call_back_func func, int arg);
bool elem_find(struct list *plist, struct list_elem *obj_elem);
#endif