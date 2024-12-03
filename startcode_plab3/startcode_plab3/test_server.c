/**
 * \author {AUTHOR}
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "config.h"
#include "lib/tcpsock.h"
#include <pthread.h>

pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;
int conn_counter = 0; // Global connection counter
int conn_terminations = 0; //counter for closed connections


/**
 * Implements a sequential test server (only one connection at the same time)
 */

void * client_handler(void* arg) {

    tcpsock_t* client = (tcpsock_t*)arg;
    sensor_data_t data;
    int bytes, result;

    do { //client handler
        // read sensor ID
        bytes = sizeof(data.id);
        result = tcp_receive(client, (void *) &data.id, &bytes);
        // if (result != TCP_NO_ERROR) break;
        // read temperature
        bytes = sizeof(data.value);
        result = tcp_receive(client, (void *) &data.value, &bytes);
        // if (result != TCP_NO_ERROR) break;
        // read timestamp
        bytes = sizeof(data.ts);
        result = tcp_receive(client, (void *) &data.ts, &bytes);
        // if (result != TCP_NO_ERROR) break;

        if ((result == TCP_NO_ERROR) && bytes) {
            printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value,
                   (long int) data.ts);
        }

    } while (result == TCP_NO_ERROR);

    if (result == TCP_CONNECTION_CLOSED)
        printf("Peer has closed connection\n");
    else
        printf("Error occured on connection to peer\n");

    // close client
    tcp_close(&client);

    //inc counter
    pthread_mutex_lock(&conn_mutex);
    conn_terminations++;
    pthread_mutex_unlock(&conn_mutex);

    return NULL;
}

int main(int argc, char *argv[]) {
    tcpsock_t *server, *client;
    pthread_t thread_id;
    
    if(argc < 3) {
    	printf("Please provide the right arguments: first the port, then the max nb of clients");
    	return -1;
    }
    
    int MAX_CONN = atoi(argv[2]);
    int PORT = atoi(argv[1]);

    printf("Test server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);

    do { //do while
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) exit(EXIT_FAILURE);
        printf("Incoming client connection\n");

        pthread_mutex_lock(&conn_mutex);
        conn_counter++;
        pthread_mutex_unlock(&conn_mutex);

        //check if max connections reached
        if (conn_counter<=MAX_CONN) {
            //add client and create thread
            //create new thread
            if (pthread_create(&thread_id, NULL, client_handler, (void*)client) != 0) {
                perror("Failed to create thread");
                tcp_close(&client);
                continue;
            }

            //close thread after handler is done
            //join it with main thread
            pthread_join(thread_id, NULL);
        }
        else {
            //dont create new client
            printf("Max number of clients reached, new client could not init");
        }

    } while (conn_terminations < conn_counter); //change this to only test when all clients are closed


    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    printf("Test server is shutting down\n");
    return 0;
}




