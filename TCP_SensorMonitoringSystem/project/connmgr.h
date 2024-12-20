#ifndef CONNMGR_H
#define CONNMGR_H

#include "lib/tcpsock.h"
#include "config.h"

// Initializes the server and handles connections
void connmgr_listen(int port, int max_conn);

#endif // CONNMGR_H
