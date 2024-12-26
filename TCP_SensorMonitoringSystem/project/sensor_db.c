#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "sensor_db.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "sbuffer.h"
#include <pthread.h>

//////////////////////////// logger process ///////////////////////////////////////////

#define LOG_FILE "gateway.log"

#define BUFFER_SIZE 256
#define WRITE_END 1
#define READ_END 0
#define SENSOR_DB_FILENAME "sensor_db.csv"
#define APPEND_MODE true
#define WRITE_MODE false


static int sequence_number = 0;
static int pipe_fd[2];
static pid_t logger_pid = 0;

pthread_mutex_t file_write_mutex = PTHREAD_MUTEX_INITIALIZER;


void write_to_log_process(char *msg) {
    if (logger_pid == 0) {
        fprintf(stderr, "Logger process not initialized.\n");
        return;
    }

    char formatted_msg[BUFFER_SIZE];
    time_t now = time(NULL);
    snprintf(formatted_msg, BUFFER_SIZE, "%d - %s - %s", sequence_number++, strtok(ctime(&now), "\n"), msg); //formatted log msg

    // Write the formatted message to the pipe
    ssize_t bytes_written = write(pipe_fd[1], formatted_msg, strlen(formatted_msg));
    if (bytes_written == -1) {
        perror("Error writing to pipe");
        return;
    }
}


void create_log_process() {
    if (pipe(pipe_fd) == -1) {
        printf("Create pipe error\n");
        return;
    }

    logger_pid = fork();

    if (logger_pid == -1) {
        printf("Fork error\n");
        return;
    }

    if (logger_pid == 0) { // Child process
        close(pipe_fd[WRITE_END]);
        FILE *log_file = fopen(LOG_FILE, "a");

        if (!log_file) {
            printf("fopen error\n");
            return;
        }

        char buffer[BUFFER_SIZE];

        while (1) { //continuous loop keeps log child open
            ssize_t bytes_read = read(pipe_fd[0], buffer, BUFFER_SIZE - 1);
            if (bytes_read <= 0) break;

            buffer[bytes_read] = '\0';

            if (strcmp(buffer, "Data file closed\n") == 0) {
                fprintf(log_file, "Logger process terminating.\n");
                fflush(log_file);
                break;
            }

            fprintf(log_file, "%s", buffer);
            fflush(log_file);
        }

        fclose(log_file);
        close(pipe_fd[READ_END]);
        exit(0);
    }

    //parent sensor_db.c
    close(pipe_fd[READ_END]);
}

void end_log_process() {
//    if (write_to_log_process("Data file closed\n") == -1) { //TODO: add error handler
//        fprintf(stderr, "Error sending terminate message to logger.\n");
//    }

    write_to_log_process("Data file closed\n");
    write_to_log_process("ending log process\n");

    usleep(100000); // Wait for 100ms to ensure logger finishes before closing

    if (close(pipe_fd[WRITE_END]) == -1) {
        perror("Error closing pipe");
        return;
    }

    if (waitpid(logger_pid, NULL, 0) == -1) {
        perror("Error waiting for logger process to terminate");
        return;
    }

     printf("Logger process terminated successfully.\n");
}

//////////////////////////// Sensor DB process ///////////////////////////////////////////
//reader process impliments this


void * open_db() { //hosts sbuffer reader process

//    if (create_log_process() == -1) { //move to main.c
//        fprintf(stderr, "Logger creation failed.\n");
//        fflush(stderr);
//        return NULL;
//    }

    FILE *file_out = fopen(SENSOR_DB_FILENAME, WRITE_MODE ? "a" : "w"); //TODO: change to append mode, write mode testing only
    if (file_out) {
        write_to_log_process("Data file opened.\n");
    } else {
        write_to_log_process("Error opening data file.\n");
    }

    sensor_data_t data;
    sbuffer_node_struct *node = NULL;

    while (1) { //process that reads from buffer
        int read_ret;
        do {
            read_ret = sbuffer_read(&node, &data);
            usleep(10);
        } while (read_ret == SBUFFER_EMPTY);

        if (data.id == 0) { // End-of-stream marker
            static int eos_count = 0; // Track how many threads have terminated
            eos_count++;
            if (eos_count < 1) { //TODO: initial tests only for 1 reader
                sbuffer_insert(&data); // Reinsert for remaining readers
            }
            break;
        }
        pthread_mutex_lock(&file_write_mutex);
        fprintf(file_out, "%hu,%.4f,%ld\n", data.id, data.value, data.ts);
        fflush(file_out);
        pthread_mutex_unlock(&file_write_mutex);
        // printf("%hu,%.4f,%ld\n", data.id, data.value, data.ts);//use for testing
        usleep(25); // Sleep for 0.25ms
        }

    printf("closing sensor_db process"); //debug line
    close_db(file_out);
    return NULL;
}

void insert_sensor(FILE *f, sensor_id_t id, sensor_value_t value, sensor_ts_t ts) {
    if (!f) {
        write_to_log_process("File pointer null during insert.\n");
        return;
    }

    ts = time(NULL);

    // printf("Inserting: ID=%d, Value=%lf, Timestamp=%ld\n", id, value, ts); //used for error checking
    write_to_log_process("Attempting to insert data.\n");

    int ret = fprintf(f, "%" PRIu16 ", %lf, %li\n", id, value, ts);
    if (ret < 0) {
        write_to_log_process("Error writing to data file.\n");
        return;
    }

    write_to_log_process("Data inserted.\n");
    fflush(f); //clear buffer and ensure data write

}

void close_db(FILE *f) {
    if (f == NULL) {
        fprintf(stderr, "ERROR: close_db() called with NULL pointer.\n");
        write_to_log_process("ERROR: Attempted to close a NULL file pointer.\n");
        return;
    }

    if (fflush(f) == EOF) {
        fprintf(stderr, "ERROR: fflush() failed before closing the file.\n");
        write_to_log_process("ERROR: Failed to flush data before closing the file.\n");
        fclose(f);
        return;
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "ERROR: fclose() failed.\n");
        write_to_log_process("ERROR: Failed to close the data file.\n");
        return;
    }

    // write_to_log_process("Data file closed successfully.\n");

//    if (end_log_process() != 0) { //move to main.c
//        fprintf(stderr, "ERROR: Failed to terminate the logger process.\n");
//        return -1;
//    }

    write_to_log_process("DB Closed Successfully\n");
}