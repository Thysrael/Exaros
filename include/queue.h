/**
 * @file queue.h
 * @brief
 * @date 2023-03-31
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_

/*
Acronyms:
lh: list head
le: list entry
*/

#define LIST_HEAD(name, type)                      \
    struct name                                    \
    {                                              \
        struct type *lh_first; /* first element */ \
    }

#define LIST_HEAD_INITIALIZER(head) \
    {                               \
        NULL                        \
    }

// 链表项
#define LIST_ENTRY(name, type)                                        \
    struct name                                                       \
    {                                                                 \
        struct type *le_next;  /* next element */                     \
        struct type **le_prev; /* address of previous next element */ \
    }

/*
 * List functions.
 */
#define LIST_INIT(head)          \
    do {                         \
        (head)->lh_first = NULL; \
    } while (/*CONSTCOND*/ 0)

#define LIST_SWAP(dstlist, srclist, field)                             \
    do {                                                               \
        void *tmplist;                                                 \
        tmplist = (srclist)->lh_first;                                 \
        (srclist)->lh_first = (dstlist)->lh_first;                     \
        if ((srclist)->lh_first != NULL)                               \
        {                                                              \
            (srclist)->lh_first->field.le_prev = &(srclist)->lh_first; \
        }                                                              \
        (dstlist)->lh_first = tmplist;                                 \
        if ((dstlist)->lh_first != NULL)                               \
        {                                                              \
            (dstlist)->lh_first->field.le_prev = &(dstlist)->lh_first; \
        }                                                              \
    } while (/*CONSTCOND*/ 0)

#define LIST_INSERT_AFTER(listelm, elm, field)                         \
    do {                                                               \
        if (((elm)->field.le_next = (listelm)->field.le_next) != NULL) \
            (listelm)->field.le_next->field.le_prev =                  \
                &(elm)->field.le_next;                                 \
        (listelm)->field.le_next = (elm);                              \
        (elm)->field.le_prev = &(listelm)->field.le_next;              \
    } while (/*CONSTCOND*/ 0)

#define LIST_INSERT_BEFORE(listelm, elm, field)           \
    do {                                                  \
        (elm)->field.le_prev = (listelm)->field.le_prev;  \
        (elm)->field.le_next = (listelm);                 \
        *(listelm)->field.le_prev = (elm);                \
        (listelm)->field.le_prev = &(elm)->field.le_next; \
    } while (/*CONSTCOND*/ 0)

#define LIST_INSERT_HEAD(head, elm, field)                           \
    do {                                                             \
        if (((elm)->field.le_next = (head)->lh_first) != NULL)       \
            (head)->lh_first->field.le_prev = &(elm)->field.le_next; \
        (head)->lh_first = (elm);                                    \
        (elm)->field.le_prev = &(head)->lh_first;                    \
    } while (/*CONSTCOND*/ 0)

#define LIST_INSERT_TAIL(head, elm, field)                                           \
    do {                                                                             \
        if (LIST_FIRST((head)) != NULL)                                              \
        {                                                                            \
            LIST_NEXT((elm), field) = LIST_FIRST((head));                            \
            while (LIST_NEXT(LIST_NEXT((elm), field), field) != NULL)                \
            {                                                                        \
                LIST_NEXT((elm), field) = LIST_NEXT(LIST_NEXT((elm), field), field); \
            }                                                                        \
            LIST_NEXT(LIST_NEXT((elm), field), field) = (elm);                       \
            (elm)->field.le_prev = &LIST_NEXT(LIST_NEXT((elm), field), field);       \
            LIST_NEXT((elm), field) = NULL;                                          \
        }                                                                            \
        else                                                                         \
        {                                                                            \
            LIST_INSERT_HEAD((head), (elm), field);                                  \
        }                                                                            \
    } while (0)

#define LIST_REMOVE(elm, field)                       \
    do {                                              \
        if ((elm)->field.le_next != NULL)             \
            (elm)->field.le_next->field.le_prev =     \
                (elm)->field.le_prev;                 \
        *(elm)->field.le_prev = (elm)->field.le_next; \
        (elm)->field.le_next = NULL;                  \
        (elm)->field.le_prev = NULL;                  \
    } while (/*CONSTCOND*/ 0)

/*
 * Like LIST_REMOVE() but safe to call when elm is not in a list
 */
#define LIST_SAFE_REMOVE(elm, field)                      \
    do {                                                  \
        if ((elm)->field.le_prev != NULL)                 \
        {                                                 \
            if ((elm)->field.le_next != NULL)             \
                (elm)->field.le_next->field.le_prev =     \
                    (elm)->field.le_prev;                 \
            *(elm)->field.le_prev = (elm)->field.le_next; \
            (elm)->field.le_next = NULL;                  \
            (elm)->field.le_prev = NULL;                  \
        }                                                 \
    } while (/*CONSTCOND*/ 0)

/* Is elm in a list? */
#define LIST_IS_INSERTED(elm, field) ((elm)->field.le_prev != NULL)

#define LIST_FOREACH(var, head, field) \
    for ((var) = ((head)->lh_first);   \
         (var);                        \
         (var) = ((var)->field.le_next))

#define LIST_FOREACH_SAFE(var, head, field, next_var)       \
    for ((var) = ((head)->lh_first);                        \
         (var) && ((next_var) = ((var)->field.le_next), 1); \
         (var) = (next_var))

/*
 * List access methods.
 */
#define LIST_EMPTY(head) ((head)->lh_first == NULL)
#define LIST_FIRST(head) ((head)->lh_first)
#define LIST_NEXT(elm, field) ((elm)->field.le_next)

#endif /* _QUEUE_H_ */
