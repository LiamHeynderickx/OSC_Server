#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LOG_FILE "gateway.log"
#define BUFFER_SIZE 1024
#define WRITE_END 1
#define READ_END 0

static int sequence_number = 0;

FILE * log_file;

static int pipe_fd[2];
static pid_t logger_pid = 0;



int write_to_log_process(char *msg) { //write message to buffer
    if (logger_pid == 0 || write(pipe_fd[1], msg, strlen(msg)) == -1) {
        printf("Error writing to pipe\n");
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
            if (bytes_read <= 0) break;
            buffer[bytes_read] = '\0';
            if (strcmp(buffer, "TERMINATE\n") == 0) break; //close all files

            //read from buffer
            time_t now = time(NULL); // Get the current time
            fprintf(log_file, "%d - %s - %s", sequence_number++, strtok(ctime(&now), "\n"), buffer);
            fflush(log_file); // Flush to clear the buffer into the file immediately

        }

        fclose(log_file);
        close(pipe_fd[READ_END]);

        return 0; //no errors

    }
    else {
        //parent sensor process
        close(pipe_fd[READ_END]);
        return 0;
    }



}




int end_log_process() {

    int termination_status = write_to_log_process("TERMINATE\n");

    if (termination_status == -1) return -1; //error handling

    if (close(pipe_fd[WRITE_END]) == -1) {
        printf("close pipe error");
        return -1; // Return error
    }

    if (waitpid(logger_pid, NULL, 0) == -1) {
        perror("Error waiting for logger process to terminate");
        return -1; // Return error
    }

    return 0;
}


