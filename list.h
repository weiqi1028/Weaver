#ifndef LIST_H
#define LIST_H

//! The list node structure
typedef struct list_node {
    struct list_node *prev, *next;
    void *ptr;
} list_node_t;


void init_list(list_node_t *head);
void list_head_add(list_node_t *head, list_node_t *new_node);
void list_tail_add(list_node_t *head, list_node_t *new_node);
void list_remove(list_node_t *node);
int list_is_empty(list_node_t *head);

#endif
