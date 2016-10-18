#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "threadpool.h"
#include "list.h"

#define USAGE "usage: ./sort [thread_count] [input_count]\n"

struct {
    pthread_mutex_t mutex;
    int cut_thread_count;
} data_context;

static struct list_head *tmp_list;
static struct list_head *the_list = NULL;

static int thread_count = 0, data_count = 0, max_cut = 0;
static tpool_t *pool = NULL;

struct list_head *merge_list(struct list_head *list1, struct list_head *list2,
                             int (*cmp)(struct list_head *, struct list_head *))
{
    struct list_head *merged = malloc(sizeof(struct list_head));
    struct list_head *tail = &merged;
    INIT_LIST_HEAD(&merged);

    struct list_head *iter1 = list1->next;
    struct list_head *iter2 = list2->next;

    while (iter1 != list1 && iter2 != list2) {
        if (cmp(iter1, iter2) <= 0) {
            tail->next = iter1;
            iter1->prev = tail;
            iter1 = iter1->next;
        } else {
            tail->next = iter2;
            iter2->prev = tail;
            iter2 = iter2->next;
        }
    }

    tail->next = iter1 != list1 ? iter1 : iter2;
    tail->next->prev = tail;
    tail = tail->next;
    while (tail->next != list1 && tail->next != list2)
        tail = tail->next;

    tail->next = merged;
    merged->prev = tail;

    free(list1);
    free(list2);

    return merged;
}

struct list_head *merge_sort(struct list_head *list)
{
    if (list_is_singular(list))
        return list;

    struct list_head *left = list;
    struct list_head *right = malloc(sizeof(struct list_head));

    /* similar to __list_cut_position() */
    list_head *new_first = list_get_middle(list);
    right->next = new_first;
    right->prev = left->prev;
    right->prev->next = right;
    left->prev = new_first->prev;
    left->prev->next = left;
    new_first->prev = right;

    return merge_list(merge_sort(left), merge_sort(right));
}

void merge(void *data)
{
    struct list_head *_list = (struct list_head *)data;
    if (_list->size < (uint32_t) data_count) {
        pthread_mutex_lock(&(data_context.mutex));
        llist_t *_t = tmp_list;
        if (!_t) {
            tmp_list = _list;
            pthread_mutex_unlock(&(data_context.mutex));
        } else {
            tmp_list = NULL;
            pthread_mutex_unlock(&(data_context.mutex));
            task_t *_task = (task_t *) malloc(sizeof(task_t));
            _task->func = merge;
            _task->arg = merge_list(_list, _t);
            tqueue_push(pool->queue, _task);
        }
    } else {
        the_list = _list;
        task_t *_task = (task_t *) malloc(sizeof(task_t));
        _task->func = NULL;
        tqueue_push(pool->queue, _task);
        list_print(_list);
    }
}

void cut_func(void *data)
{
    llist_t *list = (llist_t *) data;
    pthread_mutex_lock(&(data_context.mutex));
    int cut_count = data_context.cut_thread_count;
    if (list->size > 1 && cut_count < max_cut) {
        ++data_context.cut_thread_count;
        pthread_mutex_unlock(&(data_context.mutex));

        /* cut list */
        int mid = list->size / 2;
        llist_t *_list = list_new();
        _list->head = list_nth(list, mid);
        _list->size = list->size - mid;
        list_nth(list, mid - 1)->next = NULL;
        list->size = mid;

        /* create new task: left */
        task_t *_task = (task_t *) malloc(sizeof(task_t));
        _task->func = cut_func;
        _task->arg = _list;
        tqueue_push(pool->queue, _task);

        /* create new task: right */
        _task = (task_t *) malloc(sizeof(task_t));
        _task->func = cut_func;
        _task->arg = list;
        tqueue_push(pool->queue, _task);
    } else {
        pthread_mutex_unlock(&(data_context.mutex));
        merge(merge_sort(list));
    }
}

static void *task_run(void *data)
{
    (void) data;
    while (1) {
        task_t *_task = tqueue_pop(pool->queue);
        if (_task) {
            if (!_task->func) {
                tqueue_push(pool->queue, _task);
                break;
            } else {
                _task->func(_task->arg);
                free(_task);
            }
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    if (argc < 3) {
        printf(USAGE);
        return -1;
    }
    thread_count = atoi(argv[1]);
    data_count = atoi(argv[2]);
    max_cut = thread_count * (thread_count <= data_count) +
              data_count * (thread_count > data_count) - 1;

    /* Read data */
    the_list = list_new();

    /* FIXME: remove all all occurrences of printf and scanf
     * in favor of automated test flow.
     */
    printf("input unsorted data line-by-line\n");
    for (int i = 0; i < data_count; ++i) {
        long int data;
        scanf("%ld", &data);
        list_add(the_list, data);
    }

    /* initialize tasks inside thread pool */
    pthread_mutex_init(&(data_context.mutex), NULL);
    data_context.cut_thread_count = 0;
    tmp_list = NULL;
    pool = (tpool_t *) malloc(sizeof(tpool_t));
    tpool_init(pool, thread_count, task_run);

    /* launch the first task */
    task_t *_task = (task_t *) malloc(sizeof(task_t));
    _task->func = cut_func;
    _task->arg = the_list;
    tqueue_push(pool->queue, _task);

    /* release thread pool */
    tpool_free(pool);
    return 0;
}
