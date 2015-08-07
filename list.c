#include "list.h"


//*************************************************
//! Initialize the list head
/*!
\param head
    \li The list head
*/
//*************************************************
void init_list(list_node_t *head) {
    head->next = head;
    head->prev = head;
}

//*************************************************
//! Add the new node between two adjacent nodes
/*!
\param new_node
    \li The new node to be added
\param prev
    \li The left node
\param next
    \li The right node
*/
//*************************************************
static inline void __list_add(list_node_t *new_node, list_node_t *prev, list_node_t *next) {
    new_node->next = next;
    next->prev = new_node;
    prev->next = new_node;
    new_node->prev = prev;
}

//*************************************************
//! Add the new node to the head
/*!
\param head
    \li The list head
\param new_node
    \li The new node to be added
*/
//*************************************************
void list_head_add(list_node_t *head, list_node_t *new_node) {
    __list_add(new_node, head, head->next);
}

//*************************************************
//! Add the new node to the tail
/*!
\param head
    \li The list head
\param new_node
    \li The new node to be added
*/
//*************************************************
void list_tail_add(list_node_t *head, list_node_t *new_node) {
    __list_add(new_node, head->prev, head);
}

//*************************************************
//! Remove the node from the list
/*!
\param node
    \li The node to be removed
*/
//*************************************************
void list_remove(list_node_t *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    free(node);
}

//*************************************************
//! Check if the list is empty
/*!
\param head
    \li The list head
\return
    \li 1 if the list is empty, 0 otherwise
*/
//*************************************************
int list_is_empty(list_node_t *head) {
    return head->next == head && head->prev == head;
}

