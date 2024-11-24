#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "sensor_db.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "config.h"
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include "logger.h"
#include <sys/wait.h>



FILE * open_db(char * filename, bool append){ //creates sensor as parent and logger as child

    if (create_log_process() == -1) {
        fprintf(stderr, "Logger creation failed.\n");
        // fflush(filename);
        return NULL;
    }

    FILE *df = fopen(filename, append ? "a" : "w");
    if (df) {
        write_to_log_process("Data file opened.\n");
    } else {
        write_to_log_process("Error opening data file.\n");
    }
    return df;

}

int insert_sensor(FILE * f, sensor_id_t id, sensor_value_t value, sensor_ts_t ts){

    if(!f) {
        write_to_log_process("File pointer null during insert.\n");
    }

    int ret = fprintf(f, "%"PRIu16", %lf, %li\n", id, value, ts);
    // fflush(f);

    if(ret < 0) {
        write_to_log_process("Error writing to data file.\n");
        return -1;
    };

    write_to_log_process("Data inserted.\n");
    fflush(f);
    return 0;

}

int close_db(FILE * f){
    if (f) {
        fclose(f);
        write_to_log_process("Data file closed.\n");
        end_log_process();
        return 0;
    }
    return -1;

}