/**
* \author Liam Heynderickx
 */


#include <unistd.h>
#include <wait.h>
#include <pthread.h>

#include "connmgr.h"
#include "sbuffer.h"
#include "datamgr.h"
#include "sensor_db.h"



int main(int argc, char *argv[]) {

    //start log and buffer
    sbuffer_init();
    create_log_process();

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <max_conn>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse port
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number: %s. Must be between 1 and 65535.\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Parse max_conn
    int max_conn = atoi(argv[2]);
    if (max_conn <= 0) {
        fprintf(stderr, "Invalid max_conn: %s. Must be a positive integer.\n", argv[2]);
        return EXIT_FAILURE;
    }

    printf("Process started on Port: %d, Max Connections: %d\n", port, max_conn);
    write_to_log_process("Port and Connections defined\n");

    pthread_t connmgr, sensor_db, datamgr;

    // Allocate and initialize the structure
    connmgr_args_t* args = malloc(sizeof(connmgr_args_t));
    if (args == NULL) {
        perror("Failed to allocate memory for connmgr_args_t");
        return EXIT_FAILURE;
    }
    args->port = port;
    args->max_conn = max_conn;


    if (pthread_create(&connmgr, NULL, connmgr_listen, (void*)args) != 0) { //connection manager thread
        perror("Failed to create connmgr_listen thread");
        free(args); // Clean up if thread creation fails
        return EXIT_FAILURE;
    }

    pthread_create(&sensor_db, NULL, open_db, NULL); //storage manager
    pthread_create(&datamgr, NULL, data_manager_init, NULL); //data manager

    write_to_log_process("Waiting for threads to join\n");

    //wait for threads to join and terminate succesfully.
    pthread_join(connmgr, NULL);
    pthread_join(sensor_db, NULL);
    pthread_join(datamgr, NULL);

    write_to_log_process("Threads joined\n");

    sbuffer_free(); // Clean up the shared buffer
    end_log_process();

    return 0;
}