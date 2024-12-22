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

/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
    struct sbuffer_node *next;  /**< a pointer to the next node*/
    sensor_data_t data;         /**< a structure containing the data */
} sbuffer_node_t;

/**
 * a structure to keep track of the buffer
 */
struct sbuffer {
    sbuffer_node_t *head;       /**< a pointer to the first node in the buffer */
    sbuffer_node_t *tail;       /**< a pointer to the last node in the buffer */
    pthread_mutex_t mutex;    // Mutex
    pthread_cond_t cond_var;  // Condition variable
};

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
    pthread_mutex_lock(&write_lock_mtx);
    sbuffer_node_t *dummy;
    if (buffer == NULL) {
        ERROR_HANDLER(buffer == NULL, "Buffer is NULL.");
    }

    while (buffer->head) {
        dummy = buffer->head;
        buffer->head = buffer->head->next;
        free(dummy);
    }

    pthread_mutex_unlock(&write_lock_mtx);
    //destroy sync variables
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->cond_var);

    free(buffer);
    buffer = NULL;
}


int sbuffer_remove(sensor_data_t *data) {
    sbuffer_node_t *dummy;
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


int sbuffer_insert(sensor_data_t *data) {
    sbuffer_node_t *dummy;
    if (buffer == NULL) return SBUFFER_FAILURE;

    dummy = malloc(sizeof(sbuffer_node_t));

    if (dummy == NULL) return SBUFFER_FAILURE;
    dummy->data = *data;
    dummy->next = NULL;

    pthread_mutex_lock(&buffer->mutex);
    if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL
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