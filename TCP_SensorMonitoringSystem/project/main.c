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
        usleep(25000); // Sleep for 25ms
    }

    //do not close file here leads to: double free or corruption (!prev)
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {

  	//ERROR_HANDLER(argc != 3, "Wrong number of arguments.");

    int port = 12345;
    int max_conn = 3;

    // Initialize shared buffer
    printf("Buffer operation initializing\n");
    sbuffer_init();

    FILE *file_out = fopen(OUTPUT_FILE, "w"); // Open shared file stream
    if (!file_out) {
        perror("Failed to open output file");
        return EXIT_FAILURE;
    }

    pthread_t writer, reader1, reader2;

    pthread_create(&writer, NULL,connmgr_listen(port,max_conn), (void *) &port);
    pthread_create(&reader1, NULL, reader_thread, (void *)file_out);
    pthread_create(&reader2, NULL, reader_thread, (void *)file_out);

    pthread_join(writer, NULL);
    pthread_join(reader1, NULL);
    pthread_join(reader2, NULL);

    fclose(file_out);

    return 0;
}