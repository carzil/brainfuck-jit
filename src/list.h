#ifndef _BF_LIST_H_
#define _BF_LIST_H_

#define bf_list_append(list, x) \
    if ((list)->first == NULL) { \
        (list)->first = (list)->last = (x); \
    } else { \
        (list)->last->next = (x); \
        (x)->prev = (list)->last; \
        (list)->last = (x); \
        (list)->last->next = NULL; \
    }

#define bf_list_remove(list, x) \
    if ((list)->first == (list)->last) { \
        (list)->first = (list)->last = NULL; \
    } else if (x->prev == NULL) { \
        (list)->first = (x)->next; \
        if ((list)->first != NULL) { \
            (list)->first->prev = NULL; \
        } \
    } else { \
        (x)->prev->next = (x)->next; \
        if ((list)->last == (x)) { \
            (list)->last = (x)->prev; \
        } else { \
            (x)->next->prev = (x)->prev; \
        } \
    }

#define bf_list_replace(list, x, new_x) \
    if ((x)->prev == NULL) { \
        (list)->first = new_x; \
        (new_x)->next = (x)->next; \
    } else { \
        (new_x)->next = (x)->next; \
        (new_x)->prev = (x)->prev; \
        (x)->prev->next = new_x; \
    } \
    if (new_x->next == NULL) { \
        (list)->last = new_x; \
    } else { \
        (new_x)->next->prev = new_x; \
    }

#define bf_is_list_empty(list) !((list)->first)

#define BF_LIST_FOREACH(list, ptr) for ((ptr) = (list)->first; (ptr); (ptr) = (ptr)->next)
#define BF_LIST_FOREACH_SAFE(list, ptr, tmp) for ((ptr) = (list)->first; (ptr) && (tmp = (ptr)->next, 1); (ptr) = tmp)

#endif