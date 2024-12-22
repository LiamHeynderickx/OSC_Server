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
sbuffer_t *shared_buffer;


int main(int argc, char *argv[]) {

  	ERROR_HANDLER(argc != 3, "Wrong number of arguments.");

    int port = atoi(argv[1]);
    int max_conn = atoi(argv[2]);

    // Initialize shared buffer
    printf("Buffer operation initializing\n");
    sbuffer_init(&shared_buffer);

    pthread_t tid[1];
    pthread_create(&tid[0], NULL,connmgr_listen(port,max_conn), (void *) &port);

    return 0;
}