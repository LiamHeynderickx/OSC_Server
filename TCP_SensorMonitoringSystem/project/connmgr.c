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
#include <stdbool.h>
#include <signal.h>
#include "sbuffer.h"
#include <bits/sigthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "sensor_db.h"


pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t conn_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;
int conn_counter = 0;         // Global connection counter
int conn_terminations = 0;    // Counter for closed connections
bool max_clients = false;
int MAX_CONN = 0;

void signal_handler(int sig) {
    printf("Thread received signal %d, exiting.\n", sig);
    write_to_log_process("Thred recieved exit signal\n");
    pthread_exit(NULL);  // Exit the thread cleanly
}




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

        //add a timeout here that closes the client after timeout is reached, TIMEOUT is defined in connmgr.h

        sbuffer_insert(&data);

    } while (result == TCP_NO_ERROR);

    if (result == TCP_CONNECTION_CLOSED){
        printf("Peer has closed connection\n");
        write_to_log_process("Peer has closed connection\n");
    }
    else{
        printf("Error occurred on connection to peer\n");
        write_to_log_process("Peer connection error\n");
    }

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



bool stop_excess_handler = false;

void* excess_client_handler(void* arg) {
    tcpsock_t* server = (tcpsock_t*)arg;
    tcpsock_t* client = NULL;

    while (true) {
        pthread_mutex_lock(&socket_mutex);
        if (stop_excess_handler) {
            pthread_mutex_unlock(&socket_mutex);
            break;
        }
        pthread_mutex_unlock(&socket_mutex);

        pthread_mutex_lock(&socket_mutex);
        int result = tcp_wait_for_connection(server, &client);
        pthread_mutex_unlock(&socket_mutex);

        if (result == TCP_NO_ERROR) {
            pthread_mutex_lock(&conn_mutex);
            if (conn_counter >= MAX_CONN) {
                printf("Rejecting excess client connection\n");
                write_to_log_process("Rejecting excess client connection\n");
                if (client) {
                    tcp_close(&client);
                    client = NULL;
                }
                printf("Closed excess client socket\n");
                write_to_log_process("Closed excess client socket\n");
            }
            pthread_mutex_unlock(&conn_mutex);
        } else if (client) {
            tcp_close(&client);
            client = NULL;
        }
    }

    printf("Excess client handler exiting.\n");
    write_to_log_process("Excess client handler terminating\n");
    return NULL;
}




void * connmgr_listen(void* arg){

  	connmgr_args_t* args = (connmgr_args_t*)arg; // Cast to connmgr_args_t*
    int PORT = args->port;
    MAX_CONN = args->max_conn;

    tcpsock_t *server, *client;
    pthread_t thread_id;

    printf("Test server is started\n");
    write_to_log_process("Test server is started\n");

    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);

    pthread_mutex_lock(&conn_mutex);

    while (conn_counter < MAX_CONN) { //has to be this otherwise termination fails
        pthread_mutex_unlock(&conn_mutex);

        // Wait for a client connection
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) {
            perror("Failed to wait for connection");
            write_to_log_process("Failed to wait for connection\n");
            continue;
        }

        printf("Incoming client connection\n");
        write_to_log_process("Incomming client connection\n");

        pthread_mutex_lock(&conn_mutex);
        if (conn_counter < MAX_CONN) {
            conn_counter++;
            pthread_mutex_unlock(&conn_mutex);

            printf("Incoming client connection succeeded\n");
            write_to_log_process("Incomming client connection succeeded\n");

            // Create a thread for the client
            if (pthread_create(&thread_id, NULL, client_handler, (void*)client) != 0) {
                perror("Failed to create thread");
                write_to_log_process("failed to create thread\n");
                tcp_close(&client);
                continue;
            }

            // Detach the thread
            pthread_detach(thread_id);
        } else { //this else statement is never reached because while loop terminates before this happens
            // Ensure the mutex is unlocked before rejecting the client
            pthread_mutex_unlock(&conn_mutex);

            // Print rejection message and close the client
            printf("Max number of clients reached. New connection rejected.\n");
            tcp_close(&client);
        }

        pthread_mutex_lock(&conn_mutex);
    }


    //excess client handler
    pthread_t excess_thread;
    pthread_create(&excess_thread, NULL, excess_client_handler, (void*)server);

    // Wait until all client threads terminate
    while (conn_terminations < MAX_CONN) {
        pthread_cond_wait(&conn_cond, &conn_mutex);
    }
    pthread_mutex_unlock(&conn_mutex);
    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    pthread_mutex_destroy(&conn_mutex);
    pthread_cond_destroy(&conn_cond);
//    pthread_cancel(excess_thread);
    // Signal the excess client handler to stop
    stop_excess_handler = true;
//    usleep(1000);
    printf("Sending SIGUSR1 to the thread.\n");

    stop_excess_handler = true;
    pthread_join(excess_thread, NULL);


    printf("Test server is shutting down\n");
    write_to_log_process("Test server is shutting down\n");
    //insert eof
    sensor_data_t data = {0};  // Create an end-of-stream marker on the stack
	sbuffer_insert(&data);
    free(args);
    return NULL;
}