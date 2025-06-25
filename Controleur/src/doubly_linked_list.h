#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H

#include <stddef.h>
#include "utils.h"

// Contains the next position (x, y) and the time of arrival
typedef struct FishNextPos {
    int x;
    int y;
    microseconds_t arrival_time;  // Time in microseconds
} FishNextPos;

typedef struct Node {
    FishNextPos data;
    struct Node* prev;
    struct Node* next;
} Node;

typedef struct DoublyLinkedList {
    Node* head;
    Node* tail;
    size_t size;
} DoublyLinkedList;

// Create and initialize a new list
DoublyLinkedList* create_list();

// Destroy the list and free memory
void destroy_list(DoublyLinkedList* list);

// Insert a node at the front
void insert_front(DoublyLinkedList* list, FishNextPos data);

// Insert a node at the back
void insert_back(DoublyLinkedList* list, FishNextPos data);

// Delete the first occurrence of a matching position
void delete_value(DoublyLinkedList* list, FishNextPos data);

// Print the list forward
void print_list_forward(const DoublyLinkedList* list);

// Print the list backward
void print_list_backward(const DoublyLinkedList* list);

// Peek at the front element without removing it
FishNextPos* peek_front(const DoublyLinkedList* list);

// Peek at the back element without removing it
FishNextPos* peek_back(const DoublyLinkedList* list);

// Get i-th element without removing it
FishNextPos* peek_at_index(const DoublyLinkedList* list, size_t index);

// Pop (remove and return) the front element
FishNextPos pop_front(DoublyLinkedList* list);

// Pop (remove and return) the back element
FishNextPos pop_back(DoublyLinkedList* list);

// Chain list2 to the end of list1
void chain_lists(DoublyLinkedList* list1, DoublyLinkedList* list2);

// Helper to compare two FishNextPos structs
int fish_next_pos_equals(FishNextPos a, FishNextPos b);

#endif // DOUBLY_LINKED_LIST_H
