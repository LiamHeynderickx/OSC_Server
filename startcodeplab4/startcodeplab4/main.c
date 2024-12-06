#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "sbuffer.h"

#define SENSOR_FILE "sensor_data" //binary data file
#define OUTPUT_FILE "sensor_data_out.csv"

sbuffer_t *shared_buffer;

//TODO add read and write threads

//reads data from file and writes to buffer
void *writer_thread(void *arg) { //*arg parameter required for the pthread_create function
    FILE *sens_file = fopen(SENSOR_FILE, "r");
    if (!sens_file) {
        perror("Failed to open sensor file");
        pthread_exit(NULL);
    }

    sensor_data_t record;
    while (fread(&record, sizeof(sensor_data_t), 1, sens_file) == 1) {
        sbuffer_insert(shared_buffer, &record);
        usleep(10000); // Sleep for 10ms
    }

    // put zero at end of stream
    record.id = 0;
    sbuffer_insert(shared_buffer, &record);

    fclose(sens_file);
    pthread_exit(NULL);
}

//reads data from the buffer and inserts it to file
void *reader_thread(void *arg) { //*arg parameter required for the pthread_create function
    FILE *file_out = fopen(OUTPUT_FILE, "a"); //a for append :)
    if (!file_out) {
        perror("Failed to open output file");
        pthread_exit(NULL);
    }

    sensor_data_t data;
    while (1) {
        sbuffer_remove(shared_buffer, &data);
        if (data.id == 0) break; // End-of-stream marker
        fprintf(file_out, "%hu,%.2f,%ld\n", data.id, data.value, data.ts);
        usleep(25000); // Sleep for 25ms
    }

    fclose(file_out);
    pthread_exit(NULL);
}

int clear_file(const char *filename) {
    FILE *file = fopen(filename, "w"); // Open in write mode truncates data in file, clearing it
    if (!file) {
        perror("Failed to open file");
        return -1;
    }
    fclose(file); // Close the file
    return 0;
}


int main(){

    //clear sensor_data_out.csv for testing purposes
    static const char *s_out = "sensor_data_out.csv";
    clear_file(s_out);

    // Initialize shared buffer
    sbuffer_init(&shared_buffer);

    pthread_t writer, reader1, reader2;

    // Create threads
    pthread_create(&writer, NULL, writer_thread, NULL);
    pthread_create(&reader1, NULL, reader_thread, NULL);
    pthread_create(&reader2, NULL, reader_thread, NULL);

    // Wait for threads to finish
    pthread_detach(writer);
    pthread_detach(reader1);
    pthread_detach(reader2);

    // Clean up
    sbuffer_free(&shared_buffer);

    return 0;
}

