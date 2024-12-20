#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "sbuffer.h"
#include <string.h>
#include <stdbool.h>

#define SENSOR_FILE "sensor_data" //binary data file
#define SENSOR_FILE_EMPTY "sensor_data_empty" //binary data file for testing only
#define OUTPUT_FILE "sensor_data_out.csv"
#define NUM_READERS 2


sbuffer_t *shared_buffer;

pthread_mutex_t file_write_mutex = PTHREAD_MUTEX_INITIALIZER;


//reads data from file and writes to buffer // replace this with connmgr
void *writer_thread(void *arg) { //*arg parameter required for the pthread_create function
    FILE *sens_file = fopen(SENSOR_FILE, "r");
    if (!sens_file) {
        perror("Error opening sensor file");
        exit(EXIT_FAILURE); // Terminate the program
    }
    // FILE *sens_file = fopen(SENSOR_FILE_EMPTY, "r"); //for testing
    if (!sens_file) {
        perror("Failed to open sensor file");
        pthread_exit(NULL);
    }

    sensor_data_t record;
    while (fread(&record, sizeof(sensor_data_t), 1, sens_file) == 1) {
        sbuffer_insert(shared_buffer, &record);
        usleep(10000); // Sleep for 10ms
        // sbuffer_remove(shared_buffer, &record);
    }

    // put zero at end of stream
    record.id = 0;
    sbuffer_insert(shared_buffer, &record);

    fclose(sens_file);
    pthread_exit(NULL);
}

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
        sbuffer_remove(shared_buffer, &data);
        if (data.id == 0) { // End-of-stream marker
            static int eos_count = 0; // Track how many threads have terminated
            eos_count++;
            if (eos_count < NUM_READERS) {
                sbuffer_insert(shared_buffer, &data); // Reinsert for remaining readers
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

int clear_file(const char *filename) {
    FILE *file = fopen(filename, "w"); // Open in write mode truncates data in file, clearing it
    if (!file) {
        perror("Failed to open file");
        return -1;
    }
    fclose(file); // Close the file
    return 0;
}


bool output_test() {

    //true indicates a failure somewhere
    static bool has_error = false;

    FILE *file_out = fopen(OUTPUT_FILE, "r");
    if (!file_out) {
        perror("Failed to open output file");
        return true;
    }

    FILE *file_test_values = fopen("sensor_data_text", "r");
    if (!file_test_values) {
        perror("Failed to open test values file");
        fclose(file_out);
        return true;
    }

    char output_line[256];
    char test_line[256];
    int line_number = 1;

    //because of the slight formatting difference between the two files we have to compare component by component

    while (fgets(output_line, sizeof(output_line), file_out) &&
           fgets(test_line, sizeof(test_line), file_test_values)) {
        // Parse the CSV line into components
        unsigned short id_out, id_test;
        float value_out, value_test;
        long timestamp_out, timestamp_test;

        if (sscanf(output_line, "%hu,%f,%ld", &id_out, &value_out, &timestamp_out) != 3) {
            printf("Error parsing output file at line %d: %s\n", line_number, output_line);
            has_error = true;
            continue;
        }

        // Parse the test file line
        if (sscanf(test_line, "%hu %f %ld", &id_test, &value_test, &timestamp_test) != 3) {
            printf("Error parsing test file at line %d: %s\n", line_number, test_line);
            has_error = true;
            continue;
        }

        // Compare parsed values
        if (id_out != id_test || value_out != value_test || timestamp_out != timestamp_test) {
            printf("Mismatch at line %d:\n", line_number);
            printf("Output: %hu,%.4f,%ld\n", id_out, value_out, timestamp_out);
            printf("Test:   %hu %.4f %ld\n", id_test, value_test, timestamp_test);
            has_error = true;
        } else {
            // printf("Line %d matches: %hu,%.4f,%ld\n", line_number, id_out, value_out, timestamp_out); //use for testing
        }

        line_number++;
    }

    // Check for extra lines in either file
    if (fgets(output_line, sizeof(output_line), file_out)) {
        printf("Extra line in output file at line %d: %s\n", line_number, output_line);
        has_error = true;
    }
    if (fgets(test_line, sizeof(test_line), file_test_values)) {
        printf("Extra line in test file at line %d: %s\n", line_number, test_line);
        has_error = true;
    }

    fclose(file_out);
    fclose(file_test_values);

    return has_error;
}




int main(){

    //clear sensor_data_out.csv for testing purposes
    static const char *s_out = OUTPUT_FILE;
    clear_file(s_out);

    // Initialize shared buffer
    printf("Buffer operation initializing\n");
    sbuffer_init(&shared_buffer);

    FILE *file_out = fopen(OUTPUT_FILE, "w"); // Open shared file stream
    if (!file_out) {
        perror("Failed to open output file");
        return EXIT_FAILURE;
    }

    pthread_t writer, reader1, reader2;
    printf("Buffer operation started, data being sent...\n");

    // Create threads
    pthread_create(&writer, NULL, writer_thread, NULL); //replace with connmgr
    pthread_create(&reader1, NULL, reader_thread, (void *)file_out);
    pthread_create(&reader2, NULL, reader_thread, (void *)file_out);
    // Wait for threads to finish
    pthread_join(writer, NULL);
    pthread_join(reader1, NULL);
    pthread_join(reader2, NULL); //error occurs on this join

    // Close the shared file after all threads finish
    fclose(file_out);

    printf("Buffer operation complete\n\n\n");


    //add tests
    //what I found while testing:
    //occasionally the order of the readings is changed, im not sure if this is something that should be protected against
    //The integrity of the readings consistently appears reliable
    //no data is lost usually

    //IMPORTANT: the gendebug makefile target has to be run to execute these tests.
    //for this reason the test is commented out
    //uncomment if you would like to test the output

//    printf("Beginning tests. If no error messages then all tests passed\n");
//    if (output_test()) {
//        printf("Error in output file\n");
//    }
//    else printf("All tests passed\n");
//    printf("Tests complete\n");
//
//    // Clean up
//    sbuffer_free(&shared_buffer);
//
//    return 0;
}