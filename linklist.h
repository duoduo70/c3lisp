#ifndef _LINKLIST_H_
#define _LINKLIST_H_

#include <stdlib.h>

#define REGISTER_LINKLIST_STRUCTS(btypename, T)                                        \
        struct btypename##LinkList {                                                   \
                struct btypename##ListNode *head;                                      \
                struct btypename##ListNode *last;                                      \
        };                                                                             \
                                                                                       \
        struct btypename##ListNode {                                                   \
                T value;                                                               \
                struct btypename##ListNode *next;                                      \
        };

#define REGISTER_LINKLIST_FUNCTIONS(btypename, stypename, T, VALUE_FREE)               \
        struct btypename##LinkList stypename##_linklist_create() {                     \
                struct btypename##LinkList tmp;                                        \
                tmp.head = NULL;                                                       \
                tmp.last = NULL;                                                       \
                return tmp;                                                            \
        }                                                                              \
                                                                                       \
        void stypename##_linklist_push(struct btypename##LinkList *linklist,           \
                                       T value) {                                      \
                if (linklist->head == NULL) {                                          \
                        struct btypename##ListNode *tmp =                              \
                            c3_malloc(sizeof(struct btypename##ListNode));             \
                        tmp->value = value;                                            \
                        tmp->next = NULL;                                              \
                        linklist->head = tmp;                                          \
                        linklist->last = tmp;                                          \
                } else {                                                               \
                        struct btypename##ListNode *tmp =                              \
                            c3_malloc(sizeof(struct btypename##ListNode));             \
                        tmp->value = value;                                            \
                        tmp->next = NULL;                                              \
                        linklist->last->next = tmp;                                    \
                        linklist->last = tmp;                                          \
                }                                                                      \
        }                                                                              \
                                                                                       \
        void stypename##_listnode_free(struct btypename##ListNode *node) {             \
                VALUE_FREE(node->value);                                               \
                c3_free(node);                                                         \
        }                                                                              \
                                                                                       \
        void stypename##_linklist_free(struct btypename##LinkList linklist) {          \
                struct btypename##ListNode *tmp = linklist.head;                       \
        start:                                                                         \
                if (tmp == NULL) {                                                     \
                        return;                                                        \
                } else {                                                               \
                        stypename##_listnode_free(tmp);                                \
                        tmp = tmp->next;                                               \
                        goto start;                                                    \
                }                                                                      \
        }

#define for_each_listnode(basebtypename, src, op)                                      \
        do {                                                                           \
                struct basebtypename##ListNode *_ = src;                               \
                if (_ != NULL) {                                                       \
                        while (1) {                                                    \
                                do                                                     \
                                        op while (0);                                  \
                                if ((_)->next == NULL) {                               \
                                        break;                                         \
                                } else {                                               \
                                        (_) = (_)->next;                               \
                                }                                                      \
                        }                                                              \
                }                                                                      \
        } while (0);

#endif