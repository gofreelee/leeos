#include "list.h"
#include "../../kernel/interrupt.h"
void list_init(struct list *plist)
{
    plist->head.prev = 0;
    plist->tail.next = 0;
    plist->head.next = &plist->tail;
    plist->tail.prev = &plist->head;
}

void list_insert_before(struct list_elem *before, struct list_elem *elem)
{
    /*关中断，保证原子性 */
    enum intr_status old_status = intr_disable();
    before->prev->next = elem;
    elem->prev = before->prev;
    elem->next = before;
    before->prev = elem;
    intr_set_status(old_status);
}

void list_push(struct list *plist, struct list_elem *elem)
{
    list_insert_before(plist->head.next, elem);
}

void list_append(struct list *plist, struct list_elem *elem)
{
    list_insert_before(&plist->tail, elem);
}

void list_remove(struct list_elem *elem)
{
    enum intr_status old_status = intr_disable();
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
    intr_set_status(old_status);
}

struct list_elem *list_pop(struct list *plist)
{
    struct list_elem *pop_elem = plist->head.next;
    list_remove(pop_elem);
    return pop_elem;
}

bool elem_find(struct list *plist, struct list_elem *obj_elem)
{
    struct list_elem *help_elem = plist->head.next;
    while (help_elem != &plist->tail)
    {
        if (help_elem == obj_elem)
            return true;
        help_elem = help_elem->next;
    }
    return false;
}

uint32_t list_len(struct list *plist)
{
    struct list_elem *help_elem = plist->head.next;
    uint32_t len = 0;
    while (help_elem != &plist->tail)
    {
        ++len;
        help_elem = help_elem->next;
    }
    return len;
}

bool list_empty(struct list *plist)
{
    return plist->head.next == &(plist->tail);
}

struct list_elem *list_traveral(struct list *plist, call_back_func func, int arg)
{
    if (list_empty(plist))
        return 0;
    struct list_elem *help_elem = plist->head.next;
    while (help_elem != &(plist->tail))
    {
        if (func(help_elem, arg))
        {
            return help_elem;
        }
        help_elem = help_elem->next;
    }
    return 0;
}