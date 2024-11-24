#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LOG_FILE "gateway.log"

static int sequence_number = 0;

FILE * log_file;

int write_to_log_process(char *msg) {
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    if (timestamp) {
        timestamp[strcspn(timestamp, "\n")] = '\0'; // Remove the newline character
    }
    int ret = fprintf(log_file, "%d - %s - %s", sequence_number++, timestamp, msg);
    fflush(log_file);
    return ret;
}



int create_log_process() {
    // Logger process
    log_file = fopen(LOG_FILE, "a");

    if (!log_file) {
        printf("Failed to open log file\n");
        return -1;
    }
    write_to_log_process("Data file opened.\n");
    return 0;
}



int end_log_process() {
    if(log_file == NULL){
        printf("ERROR: end_log_process() called with NULL pointer\n");
        return -1;
    }

    int result = fclose(log_file);

    if(result == 0){
        printf("Log File Closed Succesfully\n");
        write_to_log_process("Data file closed.\n");
    }
    else{
        printf("ERROR: end_log_process() failed \n");
        return -1;
    }

    return result;
}


