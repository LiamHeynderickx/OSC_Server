/**
* \author Liam Heynderickx, all function headers and descriptions written by Bert Lagaisse
 */


#include "datamgr.h"
#include "lib/dplist.h"
#include <stdint.h>
#include <time.h>
#include "config.h"



dplist_t *data_list;

typedef struct {
    sensor_id_t sensor_id;
    uint16_t room_id;
    sensor_value_t window_running_avg[RUN_AVG_LENGTH];
    int current_index; // To manage circular buffer
    int num_readings;  // Track the number of readings (up to RUN_AVG_LENGTH)
    sensor_ts_t last_modified;
} list_element;


void datamgr_print_sensors(dplist_t *data_list) { // for testing
    if (!data_list || dpl_size(data_list) == 0) {
        printf("The list is empty.\n");
        return;
    }

    for (int i = 0; i < dpl_size(data_list); i++) {
        list_element *e = (list_element *)dpl_get_element_at_index(data_list, i);
        if (e) {
            // Calculate running average
            double avg = 0;
            for (int j = 0; j < RUN_AVG_LENGTH; j++) {
                avg += e->window_running_avg[j];
            }
            avg /= RUN_AVG_LENGTH;

            // Print sensor details
            printf("Sensor ID: %u, Room ID: %u, Running Avg: %.2f, Last Modified: %ld\n",
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


/**
 *  This method holds the core functionality of your datamgr. It takes in 2 file pointers to the sensor files and parses them.
 *  When the method finishes all data should be in the internal pointer list and all log messages should be printed to stderr.
 *  \param fp_sensor_map file pointer to the map file
 *  \param fp_sensor_data file pointer to the binary data file
 */
void datamgr_parse_sensor_files(FILE *fp_sensor_map, FILE *fp_sensor_data) {
    if (!fp_sensor_map) {
        fprintf(stderr, "Error: Sensor map file pointer is NULL.\n");
        exit(EXIT_FAILURE);
    }

    if (!fp_sensor_data) {
        fprintf(stderr, "Error: Sensor data file pointer is NULL.\n");
        exit(EXIT_FAILURE);
    }

    // data_list = dpl_create(element_free); CREATE LIST HERE
    data_list = dpl_create(element_copy, element_free, element_compare);

    //read sensor map (works)
    uint16_t roomID, sensorID;

    //initialize list with sensor ids and room ids then add data after
    while (fscanf(fp_sensor_map, "%hu %hu", &roomID, &sensorID) == 2) {
        // printf("Room ID: %u, Sensor ID: %u\n", roomID, sensorID);

        // printf("Size of list_element: %zu bytes\n", sizeof(list_element));

        list_element *e = (list_element *)malloc(sizeof(list_element)); //////////////error on this line

        if (!e) {
            fprintf(stderr, "Error: Memory allocation for list_element failed.\n");
            exit(EXIT_FAILURE);
        }

        e->sensor_id = sensorID;
        e->room_id = roomID;
        e->current_index = 0;
        e->num_readings = 0;
        for (int i = 0; i < RUN_AVG_LENGTH; i++) {
            e->window_running_avg[i] = 0.0;
        }
        e->last_modified = 0;

        //ADD LIST ELEMENT
        dpl_insert_at_index(data_list, e, 0, false); //puts new element at front of list
    }

    //now i need to add in measurements, running average and last modified from sensors_data
    // Read sensor data


    // datamgr_print_sensors(data_list);

}


/**
 * This method should be called to clean up the datamgr, and to free all used memory.
 * After this, any call to datamgr_get_room_id, datamgr_get_avg, datamgr_get_last_modified or datamgr_get_total_sensors will not return a valid result
 */
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



