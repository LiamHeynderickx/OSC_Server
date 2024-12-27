#ifndef CONNMGR_H
#define CONNMGR_H

#include "lib/tcpsock.h"
#include "config.h"

#ifndef TIMEOUT
#error TIMEOUT not set
#endif

// Initializes the server and handles connections
void * connmgr_listen(void* arg);

#endif // CONNMGR_H
