/**
* \author Liam Heynderickx, all function headers and descriptions written by Bert Lagaisse
 */


#include "datamgr.h"
#include "lib/dplist.h"
#include <stdint.h>
#include <time.h>
#include "config.h"
#include "sbuffer.h"
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>



#define SENSOR_MAP "room_sensor.map"

static pthread_mutex_t file_write_mutex = PTHREAD_MUTEX_INITIALIZER;

static dplist_t *data_list;

typedef struct {
    sensor_id_t sensor_id;
    uint16_t room_id;
    sensor_value_t window_running_avg[RUN_AVG_LENGTH];
    // int current_index; // To manage circular buffer
    // int num_readings;  // Track the number of readings (up to RUN_AVG_LENGTH)
    sensor_ts_t last_modified;
    sensor_value_t running_avg_value; //used for debug purposes
} list_element;

static sensor_value_t get_running_avg(sensor_value_t const window_running_avg[RUN_AVG_LENGTH]) {
    float sum = 0;
    for (int i = 0; i < RUN_AVG_LENGTH; i++) {
        sum += window_running_avg[i];
    }
    return sum / (float)RUN_AVG_LENGTH;
}


static void datamgr_print_sensors() { // for testing, local to datamgr.c
    if (!data_list || dpl_size(data_list) == 0) {
        printf("The list is empty.\n");
        return;
    }

    for (int i = 0; i < dpl_size(data_list); i++) {
        list_element *e = (list_element *)dpl_get_element_at_index(data_list, i);
        if (e) {
            // Calculate running average
            sensor_value_t avg = get_running_avg(e->window_running_avg);

            // Print sensor details
            printf("Sensor ID: %u, Room ID: %u, Running Avg: %.4f, Last Modified: %ld\n",
                   e->sensor_id,
                   e->room_id,
                   avg,
                   (long)e->last_modified);  // Cast time_t to long for portability
        } else {
            printf("Error: NULL element at index %d.\n", i);
        }
    }
}



void element_free(void **element) {
    if (element && *element) {
        free(*element);
        *element = NULL;
    }
}

//other callback functions
int element_compare(void *x, void *y) {
    if (!x || !y) return 0;

    sensor_id_t id1 = ((list_element *)x)->sensor_id;
    sensor_id_t id2 = ((list_element *)y)->sensor_id;

    if (id1 < id2) return -1;
    if (id1 > id2) return 1;
    return 0;
}

void *element_copy(void *element) {
    if (!element) return NULL;

    // Allocate memory for the new element
    list_element *copy = malloc(sizeof(list_element));

    if (!copy) {
        fprintf(stderr, "Error: Memory allocation failed in element_copy.\n");
        exit(EXIT_FAILURE);
    }

    // Perform a shallow copy of the element
    //this is sufficient as it only contains values and arrays stored directly within the structure
    *copy = *(list_element *)element;

    return (void *)copy;
}

void * data_manager_init(){

    FILE *fp_sensor_map = fopen(SENSOR_MAP, "r");

    if (!fp_sensor_map) {
        fprintf(stderr, "Error: Sensor map file pointer is NULL.\n");
        exit(EXIT_FAILURE);
    }

    data_list = dpl_create(element_copy, element_free, element_compare);
    uint16_t roomID, sensorID;

    //initialize list with sensor ids and room ids then add data after
    while (fscanf(fp_sensor_map, "%hu %hu", &roomID, &sensorID) == 2) {

        list_element *e = (list_element *)malloc(sizeof(list_element));

        if (!e) {
            fprintf(stderr, "Error: Memory allocation for list_element failed.\n");
            exit(EXIT_FAILURE);
        }

        e->sensor_id = sensorID;
        e->room_id = roomID;

        for (int i = 0; i < RUN_AVG_LENGTH; i++) {
            e->window_running_avg[i] = ((float) (SET_MAX_TEMP + SET_MIN_TEMP)) / 2.0; //set values in range so averaage moves from here
        }
        e->last_modified = 0;

        //ADD LIST ELEMENT
        dpl_insert_at_index(data_list, e, 0, false); //puts new element at front of list
    }

    sensor_data_t data;
    sbuffer_node_t *node = NULL; //This makes the datamgr remember its last position.


    FILE *file_out_dmgr = fopen("data_mgr_read.csv", false ? "a" : "w"); //TODO: change to append mode, write mode testing only

    while (1) { //process that reads from buffer //temp - just for buffer testing
        sbuffer_read(&node, &data);
        if (data.id == 0) { // End-of-stream marker
            static int eos_count = 0; // Track how many threads have terminated
            eos_count++;
            if (eos_count < 1) { //TODO: initial tests only for 1 reader
                sbuffer_insert(&data); // Reinsert for remaining readers
            }
            break;
        }
        pthread_mutex_lock(&file_write_mutex);
        fprintf(file_out_dmgr, "%hu,%.4f,%ld\n", data.id, data.value, data.ts);
        fflush(file_out_dmgr);
        pthread_mutex_unlock(&file_write_mutex);
        // printf("%hu,%.4f,%ld\n", data.id, data.value, data.ts);//use for testing
        usleep(25); // Sleep for 0.25ms
    }

    fclose(file_out_dmgr);
    printf("closing sensor_db process"); //debug line
    return NULL;
}



void datamgr_free() {

    if (!data_list) {
        fprintf(stderr, "Error: data_list is NULL during free.\n");
        exit(EXIT_FAILURE);
    }

    dpl_free(&data_list, true);

}


/**
 * Gets the room ID for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the corresponding room id
 */
uint16_t datamgr_get_room_id(sensor_id_t sensor_id) {
    return 0;
}


/**
 * Gets the running AVG of a certain senor ID (if less then RUN_AVG_LENGTH measurements are recorded the avg is 0)
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the running AVG of the given sensor
 */
// sensor_value_t datamgr_get_avg(sensor_id_t sensor_id);


/**
 * Returns the time of the last reading for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the last modified timestamp for the given sensor
 */
// time_t datamgr_get_last_modified(sensor_id_t sensor_id);


/**
 *  Return the total amount of unique sensor ID's recorded by the datamgr
 *  \return the total amount of sensors
 */
int datamgr_get_total_sensors() {
    return 0;
}