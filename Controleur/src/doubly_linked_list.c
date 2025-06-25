#include "doubly_linked_list.h"
#include <stdio.h>
#include <stdlib.h>

DoublyLinkedList* create_list() {
    DoublyLinkedList* list = (DoublyLinkedList*)malloc(sizeof(DoublyLinkedList));
    if (!list) return NULL;
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

void destroy_list(DoublyLinkedList* list) {
    if (!list) return;
    Node* current = list->head;
    while (current) {
        Node* next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

void insert_front(DoublyLinkedList* list, FishNextPos data) {
    if (!list) return;
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) return;
    new_node->data = data;
    new_node->prev = NULL;
    new_node->next = list->head;

    if (list->head)
        list->head->prev = new_node;
    else
        list->tail = new_node; // First node

    list->head = new_node;
    list->size++;
}

void insert_back(DoublyLinkedList* list, FishNextPos data) {
    if (!list) return;
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) return;
    new_node->data = data;
    new_node->next = NULL;
    new_node->prev = list->tail;

    if (list->tail)
        list->tail->next = new_node;
    else
        list->head = new_node; // First node

    list->tail = new_node;
    list->size++;
}

int fish_next_pos_equals(FishNextPos a, FishNextPos b) {
    return (a.x == b.x) && (a.y == b.y) && (a.arrival_time == b.arrival_time);
}

void delete_value(DoublyLinkedList* list, FishNextPos data) {
    if (!list) return;
    Node* current = list->head;
    while (current) {
        if (fish_next_pos_equals(current->data, data)) {
            if (current->prev)
                current->prev->next = current->next;
            else
                list->head = current->next;

            if (current->next)
                current->next->prev = current->prev;
            else
                list->tail = current->prev;

            free(current);
            list->size--;
            return;
        }
        current = current->next;
    }
}

void print_list_forward(const DoublyLinkedList* list) {
    if (!list) return;
    Node* current = list->head;
    printf("List (forward):\n");
    while (current) {
        printf("  (x=%d, y=%d, arrival_time=%ld)\n",
               current->data.x, current->data.y, (long)current->data.arrival_time);
        current = current->next;
    }
}

void print_list_backward(const DoublyLinkedList* list) {
    if (!list) return;
    Node* current = list->tail;
    printf("List (backward):\n");
    while (current) {
        printf("  (x=%d, y=%d, arrival_time=%ld)\n",
               current->data.x, current->data.y, (long)current->data.arrival_time);
        current = current->prev;
    }
}

FishNextPos* peek_front(const DoublyLinkedList* list) {
    if (!list || !list->head) return NULL;
    return &(list->head->data);
}

FishNextPos* peek_back(const DoublyLinkedList* list) {
    if (!list || !list->tail) return NULL;
    return &(list->tail->data);
}

FishNextPos* peek_at_index(const DoublyLinkedList* list, size_t index) {
    if (!list || index >= list->size) return NULL;

    Node* current = list->head;
    size_t i = 0;

    while (current && i < index) {
        current = current->next;
        i++;
    }

    return current ? &(current->data) : NULL;
}

FishNextPos pop_front(DoublyLinkedList* list) {
    FishNextPos dummy = {0, 0, 0};
    if (!list || !list->head) return dummy;

    Node* front = list->head;
    FishNextPos data = front->data;

    list->head = front->next;
    if (list->head)
        list->head->prev = NULL;
    else
        list->tail = NULL; // List became empty

    free(front);
    list->size--;

    return data;
}

void chain_lists(DoublyLinkedList* list1, DoublyLinkedList* list2) {
    if (!list1 || !list2 || list2->size == 0) return;

    if (list1->size == 0) {
        // If list1 is empty, just point it to list2
        list1->head = list2->head;
        list1->tail = list2->tail;
    } else {
        // Connect tail of list1 to head of list2
        list1->tail->next = list2->head;
        list2->head->prev = list1->tail;
        list1->tail = list2->tail;
    }

    list1->size += list2->size;

    // Empty list2
    list2->head = NULL;
    list2->tail = NULL;
    list2->size = 0;
}

FishNextPos pop_back(DoublyLinkedList* list) {
    FishNextPos dummy = {0, 0, 0};
    if (!list || !list->tail) return dummy;

    Node* back = list->tail;
    FishNextPos data = back->data;

    list->tail = back->prev;
    if (list->tail)
        list->tail->next = NULL;
    else
        list->head = NULL; // List became empty

    free(back);
    list->size--;

    return data;
}
