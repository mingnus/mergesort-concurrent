#include "genlist.h"
#include <stdlib.h>

void list_append(struct list_head *head, void* data)
{
    struct list_node *n = malloc(sizeof(struct list_node));
    INIT_LIST_HEAD(&n->list);
    n->data = data;
    list_add_tail(head, &n->list);
}

/* Using Floyd's cycle-finding algorithm */
struct list_head *list_get_middle(struct list_head* head)
{
    struct list_head *slow = head;
    struct list_head *fast = head->next;
    while (slow != fast) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
}

void list_foreach(struct list_head *head, void (*func)(void*, void*), void *user_data)
{
    struct list_head *iter = head->next;
    list_for_each(iter, head)
    (*func)(container_of(iter, struct list_node, list)->data, user_data);
}

