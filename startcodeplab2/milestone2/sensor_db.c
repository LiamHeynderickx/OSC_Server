#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "sensor_db.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "config.h"
#include <inttypes.h>


FILE * open_db(char * filename, bool append){

    FILE * fp;

    if(append) fp = fopen(filename, "a");
    else fp = fopen(filename, "w");

    return fp;

}

int insert_sensor(FILE * f, sensor_id_t id, sensor_value_t value, sensor_ts_t ts){
    int ret = fprintf(f, "%"PRIu16", %lf, %li\n", id, value, ts);
    fflush(f);
    return ret;
}

int close_db(FILE * f){
    if(f == NULL){
        printf("ERROR: close_db() called with NULL pointer\n");
        return -1;
    }

    int result = fclose(f);

    if(result == 0){
        printf("File Closed Succesfully\n");
    }
    else{
        printf("ERROR: close_db() failed \n");
    }

    return result;
}