#ifndef LIST_H_
#define LIST_H_

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
    if (x->prev == NULL) { \
        list->first = (x)->next; \
        if ((x)->next != NULL) { \
            (x)->next->prev = NULL; \
        } \
    } else { \
        (x)->prev->next = (x)->next; \
        if (x->prev->next == NULL) { \
            (list)->last = (x)->prev; \
        } else { \
            (x)->next->prev = (x)->prev; \
        } \
    }

#define bf_list_replace(list, x, new_x) \
    if (x->prev == NULL) { \
        list->first = new_x; \
    } else { \
        x->prev->next = new_x; \
        new_x->next = x->next; \
        new_x->prev = prev; \
        if (new_x->next == NULL) { \
            (list)->last = (new_x); \
        } else { \
            (new_x)->next->prev = (new_x); \
        } \
    }

#define bf_is_list_empty(list) (list)->first == NULL

#define BF_LIST_FOREACH(list, ptr) for (ptr = (list)->first; (ptr) != NULL; ptr = (ptr)->next)
#define BF_LIST_FOREACH_SAFE(list, ptr, tmp) for (ptr = (list)->first; (ptr) != NULL && (tmp = (ptr)->next, 1); ptr = tmp)

#endif