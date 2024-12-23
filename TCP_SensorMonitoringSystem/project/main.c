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
//#include "datamgr.h"

#define OUTPUT_FILE "sensor_data_out.csv"
#define NUM_READERS 2

pthread_mutex_t file_write_mutex = PTHREAD_MUTEX_INITIALIZER;



//reads data from the buffer and inserts it to file
void *reader_thread(void *arg) { //*arg parameter required for the pthread_create function
    // FILE *file_out = fopen(OUTPUT_FILE, "w"); //opening in w creates independent file streams causing sync errors
    FILE *file_out = (FILE *)arg;
    sensor_data_t data;

    if (!file_out) {
        perror("Failed to open output file");
        pthread_exit(NULL);
    }

    while (1) {
        sbuffer_remove(&data);
        if (data.id == 0) { // End-of-stream marker
            static int eos_count = 0; // Track how many threads have terminated
            eos_count++;
            if (eos_count < NUM_READERS) {
                sbuffer_insert(&data); // Reinsert for remaining readers
            }
            break;
        }
        pthread_mutex_lock(&file_write_mutex);
        fprintf(file_out, "%hu,%.4f,%ld\n", data.id, data.value, data.ts);
        fflush(file_out);
        pthread_mutex_unlock(&file_write_mutex);
        // printf("%hu,%.4f,%ld\n", data.id, data.value, data.ts);//use for testing
        usleep(2500); // Sleep for 25ms
    }

    //do not close file here leads to: double free or corruption (!prev)
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {

  	int port = 12345;
    int max_conn = 3;

    pthread_t writer, reader1, reader2;

    sbuffer_init();

    // Allocate and initialize the structure
    connmgr_args_t* args = malloc(sizeof(connmgr_args_t)); //memory leak on this line
    if (args == NULL) {
        perror("Failed to allocate memory for connmgr_args_t");
        return EXIT_FAILURE;
    }
    args->port = port;
    args->max_conn = max_conn;

   	printf("port = %d and max_conn = %d\n", port, max_conn);

    // Create the thread
    if (pthread_create(&writer, NULL, connmgr_listen, (void*)args) != 0) {
        perror("Failed to create connmgr_listen thread");
        free(args); // Clean up if thread creation fails
        return EXIT_FAILURE;
    }

    FILE *file_out = fopen(OUTPUT_FILE, "w"); // Open shared file stream
    if (!file_out) {
        perror("Failed to open output file");
        return EXIT_FAILURE;
    }


    pthread_create(&reader1, NULL, reader_thread, (void*)file_out);
    pthread_create(&reader2, NULL, reader_thread, (void*)file_out);

    printf("waiting for threads to join\n");

    pthread_join(writer, NULL);
    printf("w\n");
    pthread_join(reader1, NULL);
    printf("r1\n");
    pthread_join(reader2, NULL);
    printf("r2\n");

    fclose(file_out);
    printf("f\n");
    sbuffer_free(); // Clean up the shared buffer
    printf("b\n");

//    free(args); //already freed in connmgr

    return 0;
}
