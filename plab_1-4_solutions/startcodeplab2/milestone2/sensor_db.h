/**
 * \author Bert Lagaisse
 */

#ifndef _SENSOR_DB_H_
#define _SENSOR_DB_H_

#include <stdio.h>
#include <stdlib.h>
#include "config.h"

typedef enum {
    LOG_NEW_FILE,
    LOG_FILE_OPEN,
    LOG_FILE_CLOSE,
    LOG_FILE_INSERT,
    LOG_FILE_WRITE_ERROR
} log_event_codes;

#include <stdbool.h>
FILE * open_db(char * filename, bool append);
int insert_sensor(FILE * f, sensor_id_t id, sensor_value_t value, sensor_ts_t ts);
int close_db(FILE * f);


#endif /* _SENSOR_DB_H_ */