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


int main(int argc, char *argv[]) {

  	ERROR_HANDLER(argc != 3, "Wrong number of arguments.");

    int port = atoi(argv[1]);
    int max_conn = atoi(argv[2]);

    connmgr_listen(port, max_conn);

    return 0;
}