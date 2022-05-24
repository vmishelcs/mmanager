#ifndef __LIST_H__
#define __LIST_H__

/*
 * Interface for a singly linked list.
 */

struct list_node
{
    void *address;
    struct list_node *next;
};

// Initializes the linked list and returns the pointer to the head node.
struct list_node *list_initialize(void);

// Allocates memory on the heap for a new list_node with specified address.
struct list_node *list_create_node(void *address);

// Adds `node` to the list pointed to by `head_reference`.
// The new node inserted immediately after the head node.
void list_add_node(struct list_node **head_reference, struct list_node *node);

// Returns the number of nodes in the list.
int list_size(struct list_node *head);

// Returns the first list_node with the specified `address`. Otherwise, returns NULL.
struct list_node *list_search(struct list_node *head, void *address);

// Removes the node specified by `node` from the list without freeing memory.
// Returns `node`, or NULL if `node` could not be found.
struct list_node *list_remove_node(struct list_node **head_reference, struct list_node *node);

// Removes the node specified by `node` from the list and frees memory occupied
// by `node`. Does nothing if `node` is not in the list.
void list_delete_node(struct list_node **head_reference, struct list_node *node);

// Sorts the list in ascending order.
void list_sort(struct list_node **head_reference);

// Returns the predecessor node to `node`. Returns NULL if no predecessor was found.
// Assumes that `node` is in the list.
struct list_node *list_find_predecessor(struct list_node *head, struct list_node *node);

// Deallocates list pointed to by `head_reference`.
void list_destroy(struct list_node **head_reference);

// Prints the list contents for debugging.
void list_print(struct list_node *head);

// Returns an array of addresses in the same order as they appear in the list.
void **list_to_array(struct list_node *head);

#endif // __LIST_H__
