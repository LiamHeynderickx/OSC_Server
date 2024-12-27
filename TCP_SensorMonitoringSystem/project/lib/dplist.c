/**
* \author Liam Heynderickx
 */


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"



/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;
    void *(*element_copy)(void *src_element);
    void (*element_free)(void **element);
    int (*element_compare)(void *x, void *y);
};


dplist_t *dpl_create(// callback functions
        void *(*element_copy)(void *src_element),
        void (*element_free)(void **element),
        int (*element_compare)(void *x, void *y)
) {
    dplist_t *list;
    list = malloc(sizeof(struct dplist));
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {

    //done: add your code here
    if (list == NULL || *list == NULL) return;

    dplist_node_t *curr = (*list)->head;
    dplist_node_t *next;


    while(curr != NULL) {
        next = curr->next;
        if (curr->element != NULL && free_element && (*list)->element_free != NULL) {
            (*list)->element_free(&curr->element);
        }
        free(curr);
        curr = next;
    }

    free(*list);
    *list = NULL;


}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {

     dplist_node_t *ref_at_index, *list_node;
     if (list == NULL) return NULL;

     list_node = malloc(sizeof(dplist_node_t));
     if (list_node == NULL) return NULL;
     list_node->element = (insert_copy && list->element_copy != NULL) ? list->element_copy(element) : element;

     // pointer drawing breakpoint
     if (list->head == NULL) { // covers case 1
         list_node->prev = NULL;
         list_node->next = NULL;
         list->head = list_node;
         // pointer drawing breakpoint
     } else if (index <= 0) { // covers case 2
         list_node->prev = NULL;
         list_node->next = list->head;
         list->head->prev = list_node;
         list->head = list_node;
         // pointer drawing breakpoint
     } else {
         ref_at_index = dpl_get_reference_at_index(list, index);
         assert(ref_at_index != NULL);
         // pointer drawing breakpoint
         if (index < dpl_size(list)) { // covers case 4
             list_node->prev = ref_at_index->prev;
             list_node->next = ref_at_index;
             ref_at_index->prev->next = list_node;
             ref_at_index->prev = list_node;
             // pointer drawing breakpoint
         } else { // covers case 3
             assert(ref_at_index->next == NULL);
             list_node->next = NULL;
             list_node->prev = ref_at_index;
             ref_at_index->next = list_node;
             // pointer drawing breakpoint
         }
     }
     return list;

}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {

    if (list == NULL || list->head == NULL) return list;
    int count = 0;


    dplist_node_t *curr = list->head;
    if (index <= 0) {
        list->head = curr->next;
        if (list->head != NULL) {
            list->head->prev = NULL;
        }
        if (free_element && list->element_free) list->element_free(&curr->element);
        free(curr); // free the current node
        return list;
    }

    while (curr->next != NULL && count < index) {
        curr = curr->next;
        count++;
    }

    if (curr != NULL) {
        if (free_element && list->element_free) {
            list->element_free(&curr->element);
        }
        if (curr->prev != NULL) {
            curr->prev->next = curr->next;
        }
        if (curr->next != NULL) {
            curr->next->prev = curr->prev;
        }
        if (curr == list->head) {
            list->head = curr->next;
        }
        free(curr);
    }
    return list;

}


int dpl_size(dplist_t *list) {

    if(list == NULL) {
        return -1;
    }

    dplist_node_t *current = list->head;
    dplist_node_t *next_node;
    int size = 0;

    while (current != NULL) {
        size++;
        next_node = current->next; // Store the next node
        current = next_node; // Move to the next node
    }

    return size;
}

void *dpl_get_element_at_index(dplist_t *list, int index) {

    dplist_node_t *nodeAtIndex = dpl_get_reference_at_index(list, index);
    return (nodeAtIndex != NULL) ? nodeAtIndex->element : NULL;

}

int dpl_get_index_of_element(dplist_t *list, void *element) {

    if (list == NULL || list->head == NULL) return -1;

    dplist_node_t *curr = list->head;
    int index = 0;

    while(curr != NULL) {
        if (list->element_compare(curr->element, element) == 0) {
            return index;
        }
        curr = curr->next;
        index++;
    }

    return -1;
}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {

    dplist_node_t *dummy = NULL;
    int count = 0;

    if (list == NULL || list->head == NULL) return NULL;

    dummy = list->head;

    if (index <= 0) {return dummy;}

    while (dummy->next != NULL && count < index) {
        dummy = dummy->next;
        count++;
    }

    return dummy;

}

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {

    if (list == NULL || reference == NULL) return NULL;

    dplist_node_t *curr = list->head;
    while (curr != NULL) {
        if (curr == reference) return curr->element;
        curr = curr->next;
    }
    return NULL;
}