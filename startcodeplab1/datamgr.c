/**
* \author Liam Heynderickx, all function headers and descriptions written by Bert Lagaisse
 */


#include "datamgr.h"
#include "lib/dplist.h"
#include <stdint.h>
#include <time.h>
#include "config.h"


typedef struct {
    sensor_id_t sensor_id;
    uint16_t room_id;
    sensor_value_t window_running_avg[RUN_AVG_LENGTH];
    sensor_ts_t last_modified;
} list_element;

void element_free(void **element) {
    free(*element);
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
        exit(1);
    }

    if (!fp_sensor_data) {
        fprintf(stderr, "Error: Sensor data file pointer is NULL.\n");
        exit(1);
    }

    // data_list = dpl_create(element_free);

    //read sensor map (works)
    uint16_t roomID, sensorID;

    //initialize list with sensor ids and room ids then add data after
    while (fscanf(fp_sensor_map, "%hu %hu", &roomID, &sensorID) == 2) {
        printf("Room ID: %u, Sensor ID: %u\n", roomID, sensorID);

        list_element *e = malloc(sizeof(list_element));

        e->sensor_id = sensorID;
        e->room_id = roomID;

    }


}


/**
 * This method should be called to clean up the datamgr, and to free all used memory.
 * After this, any call to datamgr_get_room_id, datamgr_get_avg, datamgr_get_last_modified or datamgr_get_total_sensors will not return a valid result
 */
void datamgr_free() {
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
