/**
* \author Liam Heynderickx
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>


/*
 * Use for all logical and sync errors. Use for all errors that don't have standard error code.
 * This  ERROR_HANDLER code was taken from https://stackoverflow.com/questions/50338211/trying-to-use-error-handling-in-c
 * [1] Gurpreet.S, “Trying to use error handling in C,” Stack Overflow, May 14, 2018. https://stackoverflow.com/questions/50338211/trying-to-use-error-handling-in-c
 */
#define ERROR_HANDLER(condition, ...)    do {\
if (condition) {\
printf("\nError: in %s - function %s at line %d: %s\n", __FILE__, __func__, __LINE__, __VA_ARGS__);\
exit(EXIT_FAILURE);\
}\
} while(0)


typedef struct {
    int port;
    int max_conn;
} connmgr_args_t;

typedef uint16_t sensor_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;
// UTC timestamp as returned by time() - notice that the size of time_t is different on 32/64 bit machine

#pragma pack(push, 1) //removes padding causing reading errors
typedef struct {
    sensor_id_t id;
    sensor_value_t value;
    sensor_ts_t ts;
} sensor_data_t;
#pragma pack(pop)

#endif /* _CONFIG_H_ */