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
#include "config.h"

int main(int argc, char *argv[]) {
    ERROR_HANDLER(argc != 2, "Wrong number of arguments.");

}