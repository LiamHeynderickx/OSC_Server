/**
* \author Liam Heynderickx, all function headers and descriptions written by Bert Lagaisse
 */

#ifndef DATAMGR_H_
#define DATAMGR_H_

#include <stdlib.h>
#include <stdio.h>
#include "config.h"

#ifndef RUN_AVG_LENGTH
#define RUN_AVG_LENGTH 5
#endif

//temporary
#define SET_MAX_TEMP 22
#define SET_MIN_TEMP 19


#ifndef SET_MAX_TEMP
#error SET_MAX_TEMP not set
#endif

#ifndef SET_MIN_TEMP
#error SET_MIN_TEMP not set
#endif

/*
 * Use ERROR_HANDLER() for handling memory allocation problems, invalid sensor IDs, non-existing files, etc.
 */

//void datamgr_parse_sensor_files(FILE *fp_sensor_map, FILE *fp_sensor_data);
//
//void datamgr_free();
//
//uint16_t datamgr_get_room_id(sensor_id_t sensor_id);
//
//sensor_value_t datamgr_get_avg(sensor_id_t sensor_id);
//
//time_t datamgr_get_last_modified(sensor_id_t sensor_id);
//
//int datamgr_get_total_sensors();

void * data_manager_init();

#endif  //DATAMGR_H_