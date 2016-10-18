#ifndef GEN_LIST_H
#define GEN_LIST_H

#include "list.h"

struct list_node {
    void* data;
    struct list_head list;
};

void list_append(struct list_head *head, void* data);
struct list_head *list_get_middle(struct list_head* head);
void list_foreach(struct list_head *head, void (*func)(void*, void*), void *user_data);

#endif

