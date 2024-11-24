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
static int pipe_fd[2];
static pid_t logger_pid = 0;

int write_to_log_process(char *msg) {
    if (logger_pid == 0) {
        fprintf(stderr, "Logger process not initialized.\n");
        return -1;
    }

    // Prepare a formatted log message with sequence number and timestamp
    char formatted_msg[BUFFER_SIZE];
    time_t now = time(NULL);
    snprintf(formatted_msg, BUFFER_SIZE, "%d - %s - %s", sequence_number++, strtok(ctime(&now), "\n"), msg);


    // Write the formatted message to the pipe
    ssize_t bytes_written = write(pipe_fd[1], formatted_msg, strlen(formatted_msg));
    if (bytes_written == -1) {
        perror("Error writing to pipe");
        return -1;
    }

    return 0;
}


int create_log_process() {
    if (pipe(pipe_fd) == -1) {
        printf("Create pipe error\n");
        return -1;
    }

    logger_pid = fork();

    if (logger_pid == -1) {
        printf("Fork error\n");
        return -1;
    }

    if (logger_pid == 0) { // Child process
        close(pipe_fd[WRITE_END]);
        FILE *log_file = fopen(LOG_FILE, "a");

        if (!log_file) {
            printf("fopen error\n");
            return -1;
        }

        char buffer[BUFFER_SIZE];
        while (1) {
            ssize_t bytes_read = read(pipe_fd[0], buffer, BUFFER_SIZE - 1);
            if (bytes_read <= 0) break;

            buffer[bytes_read] = '\0';

            if (strcmp(buffer, "Data file closed\n") == 0) {
                fprintf(log_file, "Logger process terminating.\n");
                fflush(log_file);
                break;
            }

            fprintf(log_file, "%s", buffer); // Write the fully formatted message
            fflush(log_file);
        }


        fclose(log_file);
        close(pipe_fd[READ_END]);
        exit(0);
    }

    close(pipe_fd[READ_END]);
    return 0;
}

int end_log_process() {
    if (write_to_log_process("Data file closed\n") == -1) {
        fprintf(stderr, "Error sending terminate message to logger.\n");
        return -1;
    }

    usleep(100000); // Wait for 100ms to ensure logger finishes

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
