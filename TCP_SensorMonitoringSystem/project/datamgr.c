/**
* \author Liam Heynderickx
 */



#include "datamgr.h"
#include "lib/dplist.h"
#include <stdint.h>
#include "config.h"
#include "sbuffer.h"
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "sensor_db.h"


#define SENSOR_MAP "room_sensor.map"

static dplist_t *data_list;

typedef struct {
    sensor_id_t sensor_id;
    uint16_t room_id;
    sensor_value_t window_running_avg[RUN_AVG_LENGTH];
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


volatile sig_atomic_t keep_running = 1;

void handle_signal(int signal) {
    keep_running = 0;
}


static void datamgr_print_sensors() { // for testing
    if (!data_list || dpl_size(data_list) == 0) {
        ERROR_HANDLER(true, "The list is empty");
    }

    for (int i = 0; i < dpl_size(data_list); i++) { //prints from data list
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
        ERROR_HANDLER(true, "Memory allocation failed in element_copy");
    }

    // Perform a shallow copy of the element
    //this is sufficient as it only contains values and arrays stored directly within the structure
    *copy = *(list_element *)element;

    return (void *)copy;
}


void datamgr_free() {
    if (!data_list) {
        ERROR_HANDLER(true, "data_list is NULL during free");
    }
    dpl_free(&data_list, true);
    ERROR_HANDLER(data_list != NULL, "Error freeing list");
}


void * data_manager_init(){

    FILE *fp_sensor_map = fopen(SENSOR_MAP, "r");

    if (!fp_sensor_map) {
        ERROR_HANDLER(true, "Sensor map file pointer is null");
    }
    write_to_log_process("Sensor map opened\n");

    data_list = dpl_create(element_copy, element_free, element_compare);
    uint16_t roomID, sensorID;

    //initialize list with sensor ids and room ids then add data after
    while (fscanf(fp_sensor_map, "%hu %hu", &roomID, &sensorID) == 2) {

        list_element *e = (list_element *)malloc(sizeof(list_element));

        if (!e) {
            ERROR_HANDLER(true, "Error: Memory allocation for list_element failed.\n");
        }

        e->sensor_id = sensorID;
        e->room_id = roomID;

        for (int i = 0; i < RUN_AVG_LENGTH; i++) {
            e->window_running_avg[i] = ((float) (SET_MAX_TEMP + SET_MIN_TEMP)) / 2.0; //set values in range so average moves from here
        }
        e->last_modified = 0;

        //ADD LIST ELEMENT
        dpl_insert_at_index(data_list, e, 0, false); //puts new element at front of list
    }

    fclose(fp_sensor_map);

    write_to_log_process("Sensor mapping process complete\n");

    //now read data from buffer
    sbuffer_node_struct *node = NULL; //This makes the datamgr remember its last position.
    sensor_data_t record;

    while (1) { // waits for element
        // Get all the data in the buffer, if no data is available, sleep for 1 us waiting for new data to come in.
        do {
            int res = sbuffer_read(&node, &record);
            if (res != SBUFFER_EMPTY) break;
            if (!keep_running) return NULL; // Exit gracefully
        } while (1);

        if (record.id == 0 || !keep_running) break; // Stop if EOF in buffer.

        list_element *tmp = NULL;
        int found = false;
        for (int i = 0; i < dpl_size(data_list); ++i) {
            tmp = (list_element *) dpl_get_element_at_index(data_list, i);
            if (tmp->sensor_id == record.id) {
                found = true;
                break;
            }
        }

        if (found) { //store running avg
            for (int i = RUN_AVG_LENGTH - 1; i > 0; --i) {
                tmp->window_running_avg[i] = tmp->window_running_avg[i - 1];
            }
            //adds values for running avg
            tmp->window_running_avg[0] = record.value;
            tmp->last_modified = record.ts;

            //calculate running avg:
            sensor_value_t running_avg = get_running_avg(tmp->window_running_avg);

            tmp->running_avg_value = running_avg;

            //I have decided to leave this line as it provides a good indication of the running average and measurements taken
            //uncomment to see
            //printf("Sensor id %d, Temp Measurement: %.4f, Running Avg: %.4f Celsius, time: %ld \n", tmp->sensor_id, record.value,running_avg, tmp->last_modified); //debug line showing running average

            if (running_avg > SET_MAX_TEMP) {
                log_sensor_temperature_report(tmp->sensor_id, true, tmp->running_avg_value);
            }
            else if (running_avg < SET_MIN_TEMP) {
                log_sensor_temperature_report(tmp->sensor_id, false, tmp->running_avg_value);
            }
            else {
                //do nothing
            }
        }
        else {
            //happens when sensor ID does not exist in room sensor map
            log_invalid_sensor(record.id); //log that it is not added to list
        }


    }

    printf("\n\n\nFINAL RUNNING AVERAGES AND TIMESTAMPS:\n\n\n");
    datamgr_print_sensors(); //debug line: gives final running averages of each sensor. shows that list is correctly populated
    //I decided to keep this line as it gives a good overview of the sensors at server termination
    printf("\n\n\n\n\n\n");
    datamgr_free();
    return NULL;
}
