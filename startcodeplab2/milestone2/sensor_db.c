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

#include "logger.h"


static int pipe_fd[2];
static int logger_pid;

FILE * open_db(char * filename, bool append){ //creates sensor as parent and logger as child

    if (pipe(pipe_fd) == -1) {
        printf("pipe error");
        exit(1);
    }

    logger_pid = fork(); //creates parent and child

    if (logger_pid < 0) { //error
        printf("fork error");
        exit(1);
    }
    if (logger_pid == 0) { //child (logger) create log process
        close(pipe_fd[1]); //close write end
        int log = create_log_process();
        if (log == -1) exit(2);
        exit(0);
    }

    //parent (sensor)
    close(pipe_fd[0]); //close read end

    FILE *fp = fopen(filename, append ? "a" : "w");

    if (fp != NULL) {

    }

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

    if (logger_pid < 0) { //error
        printf("fork error");
        return -1;
    }

    int ret = 0;

    if (logger_pid > 0) {
        printf("closing db parent\n");
        ret = fclose(f);
        if (ret != 0) printf("sensor close error");
    }

    // if (logger_pid == 0 && ret == 0) { //child (logger) close log process, make sure sensor file closed
    //     printf("dgdgdfgdfgdfg");
    //     int logClose = end_log_process();
    //     if (logClose == -1) {
    //         printf("end_log_process() error");
    //         return -1;
    //     }
    // }

    // int logClose = end_log_process();
    // if (logClose == -1) {
    //     printf("log close error");
    //     return -1;
    // }

    close(pipe_fd[1]); //close parent write let child die

    return ret;
}