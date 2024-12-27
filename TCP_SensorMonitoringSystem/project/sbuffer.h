/**
* \author Liam Heynderickx
 */

#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#include <bits/pthreadtypes.h>

#include "config.h"


#define SBUFFER_FAILURE -1
#define SBUFFER_SUCCESS 0
#define SBUFFER_EMPTY 1

typedef struct sbuffer sbuffer_t;

/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
 struct sbuffer_node *next;
 sensor_data_t data;
} sbuffer_node_struct;

struct sbuffer {
 sbuffer_node_struct *head;
 sbuffer_node_struct *tail;
 pthread_mutex_t mutex;
 pthread_cond_t cond_var;
};


void sbuffer_init();
void sbuffer_free();
int sbuffer_remove(sensor_data_t *data);
int sbuffer_read(sbuffer_node_struct **node, sensor_data_t *data);
int sbuffer_insert(sensor_data_t *data);

#endif  //_SBUFFER_H_