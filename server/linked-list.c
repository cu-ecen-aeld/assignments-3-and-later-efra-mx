#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

#include "linked-list.h"

struct node *node_create(int size)
{
    struct node *node;

    if (size == 0) {
        size = sizeof(struct node);
    }
    node = (struct node *)malloc(size);
    memset(node, 0, size);
    return node;
}

void node_destroy(struct node *node)
{
    if (node == NULL) {
        return;
    }
    node->next = NULL;
    free(node);
}

int list_get_length(struct node *list)
{
    int length = 0;
    struct node *node = list;

    while (node->next != NULL) {
        length++;
        node = node->next;
    }
    return length;
}

int list_push(struct node *list, struct node *node)
{
    struct node *current = NULL;
    struct node *last_node = NULL;

    if (list == NULL) {
        return -EFAULT;
    }
    current = list;
    while (current != NULL) {
        last_node = current;
        current = current->next;
    }
    last_node->next = node;
    return 0;
}

int list_pop(struct node *list, struct node **node)
{
    struct node *current = NULL;
    struct node *last_node = NULL;
    struct node *new_last_node = NULL;

    if (list == NULL || node == NULL) {
        return -EFAULT;
    }

    *node = NULL;
    while (current != NULL) {
        new_last_node = last_node;
        last_node = current;
        current = current->next;
    }
    if (new_last_node) {
        new_last_node->next = NULL;
    }
    if (last_node) {
        *node = last_node;
    }
    return 0;

}

