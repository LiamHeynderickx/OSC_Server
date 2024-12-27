/**
 * \author {AUTHOR}
 */


#include <stdlib.h>
#include <stdio.h>
#include "sbuffer.h"
#include <pthread.h>
#include <stdbool.h>
#include "config.h"

pthread_mutex_t write_lock_mtx;

sbuffer_t *buffer;



//DONE
void sbuffer_init() {
    //initialize buff and mtx
    pthread_mutex_init(&write_lock_mtx, NULL);
    buffer = malloc(sizeof(sbuffer_t));

    ERROR_HANDLER(buffer == NULL, "Buffer malloc failed.");
    buffer->head = NULL;
    buffer->tail = NULL;


    pthread_mutex_init(&(buffer)->mutex, NULL);
    pthread_cond_init(&(buffer)->cond_var, NULL);
}



void sbuffer_free() {
    sbuffer_node_struct *current = buffer->head;
    while (current) {
        sbuffer_node_struct *next = current->next;
        free(current);
        current = next;
    }
    free(buffer);
}




int sbuffer_remove(sensor_data_t *data) { //not used
    sbuffer_node_struct *dummy;
    if (buffer == NULL) return SBUFFER_FAILURE;

    pthread_mutex_lock(&buffer->mutex);

    while (buffer->head == NULL) { // Wait if buffer is empty
        pthread_cond_wait(&buffer->cond_var, &buffer->mutex);
    }

    *data = buffer->head->data;
    dummy = buffer->head;
    if (buffer->head == buffer->tail) // buffer has only one node
    {
        buffer->head = buffer->tail = NULL;
    } else  // buffer has many nodes empty
    {
        buffer->head = buffer->head->next;
    }
    free(dummy);

    pthread_mutex_unlock(&buffer->mutex);

    return SBUFFER_SUCCESS;
}


int sbuffer_read(sbuffer_node_struct **node, sensor_data_t *data) {
    ERROR_HANDLER(buffer == NULL, "Buffer not initialized");
    if (buffer->head == NULL) return SBUFFER_EMPTY;
    if (*node == NULL) {
        *data = buffer->head->data;
        *node = buffer->head;
        return SBUFFER_SUCCESS;
    } else if ((*node)->next != NULL) {
        *data = (*node)->next->data;
        *node = (*node)->next;
        return SBUFFER_SUCCESS;
    } else {
        return SBUFFER_EMPTY;
    }
}


int sbuffer_insert(sensor_data_t *data) {
    sbuffer_node_struct *dummy;
    if (buffer == NULL) return SBUFFER_FAILURE;

    dummy = malloc(sizeof(sbuffer_node_struct));

    if (dummy == NULL) return SBUFFER_FAILURE;
    dummy->data = *data;
    dummy->next = NULL;

    pthread_mutex_lock(&buffer->mutex);
    if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL)
    {
        buffer->head = buffer->tail = dummy;
    } else // buffer not empty
    {
        buffer->tail->next = dummy;
        buffer->tail = buffer->tail->next;
    }

    //this signals pthread_cond_wait(&buffer->cond_var, &buffer->mutex); that data is ready to be removed from buffer
    pthread_cond_signal(&buffer->cond_var);

    pthread_mutex_unlock(&buffer->mutex);

    return SBUFFER_SUCCESS;
}