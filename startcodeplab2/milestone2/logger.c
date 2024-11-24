#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LOG_FILE "gateway.log"
#define BUFFER_SIZE 256
#define WRITE_END 1
#define READ_END 0

static int sequence_number = 0;

FILE * log_file;

static int pipe_fd[2];
static pid_t logger_pid = 0;



int write_to_log_process(char *msg) {
    if (logger_pid == 0) {
        fprintf(stderr, "Logger process not initialized.\n");
        return -1;
    }
    ssize_t bytes_written = write(pipe_fd[1], msg, strlen(msg));
    if (bytes_written == -1) {
        perror("Error writing to pipe");
        return -1;
    }
    return 0;
}





int create_log_process() {

    if (pipe(pipe_fd) == -1) { //check for pipe error
        printf("Create pipe error\n");
        return -1;
    }

    logger_pid = fork();

    if (logger_pid == -1) {
        printf("Fork error\n");
        return -1;
    }

    if (logger_pid == 0) { //child (logger) create log process

        close(pipe_fd[WRITE_END]);
        FILE *log_file = fopen(LOG_FILE, "a");

        if (!log_file) {
            printf("fopen error");
            // write_to_log_process("Error opening log file.\n");
            return -1;
        }


        char buffer[BUFFER_SIZE]; //set buffer size

        while (1) {
            ssize_t bytes_read = read(pipe_fd[0], buffer, BUFFER_SIZE - 1);
            if (bytes_read <= 0) break; // EOF or error

            buffer[bytes_read] = '\0';

            if (strcmp(buffer, "TERMINATE\n") == 0) {
                time_t now = time(NULL);
                fprintf(log_file, "%d - %s - Logger process terminating.\n", sequence_number++, strtok(ctime(&now), "\n"));
                fflush(log_file);
                fsync(fileno(log_file)); // Ensure log is fully written
                break;
            }

            time_t now = time(NULL);
            fprintf(log_file, "%d - %s - %s", sequence_number++, strtok(ctime(&now), "\n"), buffer);
            fflush(log_file);
            fsync(fileno(log_file)); // Ensure log is fully written
        }


        fclose(log_file);
        close(pipe_fd[READ_END]);

        return 0; //no errors

    }

    //parent
    close(pipe_fd[READ_END]);
    return 0;

}




int end_log_process() {
    if (write_to_log_process("TERMINATE\n") == -1) {
        fprintf(stderr, "Error sending terminate message to logger.\n");
        return -1;
    }

    if (close(pipe_fd[WRITE_END]) == -1) {
        perror("Error closing pipe");
        return -1;
    }

    if (waitpid(logger_pid, NULL, 0) == -1) {
        perror("Error waiting for logger process to terminate");
        return -1;
    }

    printf("Logger process terminated successfully.\n");
    return 0;
}





