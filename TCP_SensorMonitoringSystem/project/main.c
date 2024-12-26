/**
* \author Liam Heynderickx
 */


#include <unistd.h>
#include <wait.h>
#include <pthread.h>
#include <limits.h>

//#include "sensor_db.h"


#include "connmgr.h"
#include "sbuffer.h"
#include "datamgr.h"
#include "sensor_db.h"

//pthread_mutex_t file_write_mutex = PTHREAD_MUTEX_INITIALIZER; //initialized in sensor_db.c

//reads data from the buffer and inserts it to file
//replace this with sensor_db.c process
//depreciated
//void *reader_thread(void *arg) { //*arg parameter required for the pthread_create function
//    // FILE *file_out = fopen(OUTPUT_FILE, "w"); //opening in w creates independent file streams causing sync errors
//    FILE *file_out = (FILE *)arg;
//    sensor_data_t data;
//
//    if (!file_out) {
//        perror("Failed to open output file");
//        pthread_exit(NULL);
//    }
//
//    while (1) {
//        sbuffer_remove(&data);
//        if (data.id == 0) { // End-of-stream marker
//            static int eos_count = 0; // Track how many threads have terminated
//            eos_count++;
//            if (eos_count < NUM_READERS) {
//                sbuffer_insert(&data); // Reinsert for remaining readers
//            }
//            break;
//        }
//        pthread_mutex_lock(&file_write_mutex);
//        fprintf(file_out, "%hu,%.4f,%ld\n", data.id, data.value, data.ts);
//        fflush(file_out);
//        pthread_mutex_unlock(&file_write_mutex);
//        // printf("%hu,%.4f,%ld\n", data.id, data.value, data.ts);//use for testing
//        usleep(2500); // Sleep for 25ms
//    }
//
//    //do not close file here leads to: double free or corruption (!prev)
//    pthread_exit(NULL);
//}


int main(int argc, char *argv[]) {

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
    connmgr_args_t* args = malloc(sizeof(connmgr_args_t)); //memory leak on this line (fixed)
    if (args == NULL) {
        perror("Failed to allocate memory for connmgr_args_t");
        return EXIT_FAILURE;
    }
    args->port = port;
    args->max_conn = max_conn;


    // Create the thread
    if (pthread_create(&connmgr, NULL, connmgr_listen, (void*)args) != 0) {
        perror("Failed to create connmgr_listen thread");
        free(args); // Clean up if thread creation fails
        return EXIT_FAILURE;
    }


//    pthread_create(&reader1, NULL, reader_thread, (void*)file_out); //replace with sensor_db
//    pthread_create(&reader2, NULL, reader_thread, (void*)file_out);


    pthread_create(&sensor_db, NULL, open_db, NULL);

    pthread_create(&datamgr, NULL, data_manager_init, NULL);


//    printf("waiting for threads to join\n");
    write_to_log_process("Waiting for threads to join\n");

    pthread_join(connmgr, NULL);
//    printf("w\n");
    pthread_join(sensor_db, NULL);
//    printf("r1\n");
    pthread_join(datamgr, NULL);

    write_to_log_process("Threads joined\n");

    sbuffer_free(); // Clean up the shared buffer
    end_log_process();

    return 0;
}
