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
        fflush(stderr);
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

int insert_sensor(FILE *f, sensor_id_t id, sensor_value_t value, sensor_ts_t ts) {
    if (!f) {
        write_to_log_process("File pointer null during insert.\n");
        return -1;
    }

    printf("Inserting: ID=%d, Value=%lf, Timestamp=%ld\n", id, value, ts);
    write_to_log_process("Attempting to insert data.\n");

    int ret = fprintf(f, "%" PRIu16 ", %lf, %li\n", id, value, ts);
    if (ret < 0) {
        write_to_log_process("Error writing to data file.\n");
        return -1;
    }

    write_to_log_process("Data inserted.\n");
    fflush(f); // Ensure data is written
    return 0;
}



int close_db(FILE *f) {
    if (f == NULL) {
        // Log and return error if the file pointer is invalid
        fprintf(stderr, "ERROR: close_db() called with NULL pointer.\n");
        write_to_log_process("ERROR: Attempted to close a NULL file pointer.\n");
        return -1;
    }

    // Flush any pending data and close the file
    if (fflush(f) == EOF) {
        fprintf(stderr, "ERROR: fflush() failed before closing the file.\n");
        write_to_log_process("ERROR: Failed to flush data before closing the file.\n");
        fclose(f); // Try to close anyway
        return -1;
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "ERROR: fclose() failed.\n");
        write_to_log_process("ERROR: Failed to close the data file.\n");
        return -1;
    }

    // Log the successful closure of the file
    write_to_log_process("Data file closed successfully.\n");

    // Terminate the logging process
    if (end_log_process() != 0) {
        fprintf(stderr, "ERROR: Failed to terminate the logger process.\n");
        return -1;
    }

    return 0; // Indicate success
}
