/**
* \author Liam Heynderickx
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "config.h"
#include "lib/tcpsock.h"
#include <pthread.h>
#include "connmgr.h"

pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t conn_cond = PTHREAD_COND_INITIALIZER;
int conn_counter = 0;         // Global connection counter
int conn_terminations = 0;    // Counter for closed connections

void* client_handler(void* arg) {
    tcpsock_t* client = (tcpsock_t*)arg;
    sensor_data_t data;
    int bytes, result;

    do {
        // Read sensor ID
        bytes = sizeof(data.id);
        result = tcp_receive(client, (void*)&data.id, &bytes);
        if (result != TCP_NO_ERROR) break;

        // Read temperature
        bytes = sizeof(data.value);
        result = tcp_receive(client, (void*)&data.value, &bytes);
        if (result != TCP_NO_ERROR) break;

        // Read timestamp
        bytes = sizeof(data.ts);
        result = tcp_receive(client, (void*)&data.ts, &bytes);
        if (result != TCP_NO_ERROR) break;

        printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value,
               (long int)data.ts);
    } while (result == TCP_NO_ERROR);

    if (result == TCP_CONNECTION_CLOSED)
        printf("Peer has closed connection\n");
    else
        printf("Error occurred on connection to peer\n");

    // Close client socket
    tcp_close(&client);

    pthread_mutex_lock(&conn_mutex);
    conn_terminations++;
    // printf("terminations = %d\n", conn_terminations);

    // Signal the main thread if all connections are terminated
    if (conn_terminations == conn_counter) {
        pthread_cond_signal(&conn_cond);
    }
    pthread_mutex_unlock(&conn_mutex);

    return NULL;
}


void connmgr_listen(int port, int max_conn){
	tcpsock_t *server, *client;
    pthread_t thread_id;

    printf("Test server is started\n");

    if (tcp_passive_open(&server, port) != TCP_NO_ERROR) exit(EXIT_FAILURE);

    pthread_mutex_lock(&conn_mutex);

    while (conn_counter < max_conn) { //has to be this otherwise termination fails
        pthread_mutex_unlock(&conn_mutex);

        // Wait for a client connection
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) {
            perror("Failed to wait for connection");
            continue;
        }

        printf("Incoming client connection\n");

        pthread_mutex_lock(&conn_mutex);
        if (conn_counter < max_conn) {
            conn_counter++;
            pthread_mutex_unlock(&conn_mutex);

            printf("Incoming client connection succeeded\n");

            // Create a thread for the client
            if (pthread_create(&thread_id, NULL, client_handler, (void*)client) != 0) {
                perror("Failed to create thread");
                tcp_close(&client);
                continue;
            }

            // Detach the thread
            pthread_detach(thread_id);
        } else {
            // Ensure the mutex is unlocked before rejecting the client
            pthread_mutex_unlock(&conn_mutex);

            // Print rejection message and close the client
            printf("Max number of clients reached. New connection rejected.\n");
            tcp_close(&client);
        }

        pthread_mutex_lock(&conn_mutex);
    }


    // Wait until all client threads terminate
    while (conn_terminations < max_conn) {
        pthread_cond_wait(&conn_cond, &conn_mutex);
    }
    pthread_mutex_unlock(&conn_mutex);

    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    pthread_mutex_destroy(&conn_mutex);
    pthread_cond_destroy(&conn_cond);
    printf("Test server is shutting down\n");
}