#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"


// Returns true if `node` is a sentinel node, false otherwise.
static bool list_is_sentinel_node(struct list_node *node);

// Returns true if `node` is in the list, false otherwise.
static bool list_is_node_in_list(struct list_node *head, struct list_node *node);

// Swaps `left` and `right` nodes in the list: (left, ..., right) -> (right, ..., left).
static void list_swap_nodes(struct list_node **head_reference, struct list_node *left, struct list_node *right);


struct list_node *list_initialize(void) {
    struct list_node *head = list_create_node(NULL);
    struct list_node *tail = list_create_node(NULL);
    head->next = tail;
    return head;
}

struct list_node* list_create_node(void *address) {
    struct list_node *node = malloc(sizeof(struct list_node));
    node->address = address;
    node->next = NULL;
    return node;
}

void list_add_node(struct list_node **head_reference, struct list_node *node) {
    // Do not add NULL nodes or nodes with NULL addresses.
    if (node && node->address) {
        struct list_node *head = *head_reference;
        node->next = head->next;
        head->next = node;
    }
}

int list_size(struct list_node *head) {
    int count = 0;
    struct list_node *current_node = head;
    while (current_node) {
        ++count;
        current_node = current_node->next;
    }

    // Subtract 2 to account for sentinel nodes.
    return count - 2;
}

struct list_node* list_search(struct list_node *head, void *address) {
    struct list_node *current_node = head;
    while (current_node) {
        if (current_node->address == address) {
            return current_node;
        }
        current_node = current_node->next;
    }
    return NULL;
}

struct list_node *list_remove_node(struct list_node **head_reference, struct list_node *node) {
    // Check if `node` is in the list.
    if (list_is_node_in_list(*head_reference, node)) {
        // Obtain predecessor.
        struct list_node *predecessor_node = list_find_predecessor(*head_reference, node);
        predecessor_node->next = node->next;
        return node;
    }
    return NULL;
}

void list_delete_node(struct list_node **head_reference, struct list_node *node) {
    struct list_node *node_to_delete = list_remove_node(head_reference, node);
    if (node_to_delete) {
        free(node_to_delete);
    }
}

void list_sort(struct list_node **head_reference) {
    struct list_node *left_node = (*head_reference)->next;
    
    // Go through each node.
    while (left_node) {
        // Find the minimal node after `left_node`.
        struct list_node *min_node = left_node;
        struct list_node *right_node = left_node->next;
        while (right_node) {
            // Make sure `right_node` is not a sentinel node.
            // If we found a node with a smaller address, update `min_node`.
            if (!list_is_sentinel_node(right_node) && right_node->address < min_node->address) {
                min_node = right_node;
            }
            right_node = right_node->next;
        }

        // Get the next node after `left_node` to continue the sorting algorithm.
        struct list_node *next_node = left_node->next;

        // Swap `left_node` and `min_node`.
        list_swap_nodes(head_reference, left_node, min_node);

        // Move on to the next node.
        left_node = next_node;
    }
}

struct list_node* list_find_predecessor(struct list_node *head, struct list_node *node) {
    // Head node has no predecessor.
    if (head == node) {
        return NULL;
    }

    struct list_node *current_node = head;
    while (current_node->next != node) {
        current_node = current_node->next;
    }
    return current_node;
}

void list_destroy(struct list_node **head_reference)
{
    struct list_node *current_node = *head_reference;
    while (current_node->next) {
        struct list_node *next_node = current_node->next;
        free(current_node);
        current_node = next_node;
    }
    free(current_node);
}

void list_print(struct list_node *head) {
    struct list_node *current_node = head;
    while (current_node) {
        printf("%p", current_node->address);
        if (current_node->next) {
            printf(" -> ");
        }
        current_node = current_node ->next;
    }
    printf("\n");
}

void **list_to_array(struct list_node *head) {
    int n = list_size(head);
    void **arr = malloc(n * sizeof(*arr));

    int i = 0;
    struct list_node *current_node = head->next;
    while (!list_is_sentinel_node(current_node)) {
        arr[i] = current_node->address;
        ++i;
        current_node = current_node->next;
    }

    return arr;
}

static bool list_is_sentinel_node(struct list_node *node) {
    if (node->address) {
        return false;
    }
    return true;
}

bool list_is_node_in_list(struct list_node *head, struct list_node *node) {
    struct list_node *current_node = head;
    while (current_node) {
        if (current_node == node) {
            return true;
        }
        current_node = current_node->next;
    }
    return false;
}

static void list_swap_nodes(struct list_node **head_reference, struct list_node *left, struct list_node *right) {
    if (left != right) {
        struct list_node *left_predecessor = list_find_predecessor(*head_reference, left);
        struct list_node *right_predecessor = list_find_predecessor(*head_reference, right);
        struct list_node *right_successor = right->next;

        // Set left node's predecessor's successor as `right`.
        left_predecessor->next = right;

        // If `left` and `right` are subsequent nodes, set right node's successor to `left`.
        if (left->next == right) {
            right->next = left;
        }
        else {
            right->next = left->next;
            right_predecessor->next = left;
        }

        left->next = right_successor;
    }
}
