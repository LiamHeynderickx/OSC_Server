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
    write_to_log_process("Client thread received exit signal, closing\n");
    pthread_exit(NULL);  // Exit the thread cleanly
}




void* client_handler(void* arg) {
    tcpsock_t* client = (tcpsock_t*)arg;
    sensor_data_t data;
    int bytes, result;

    time_t start_time = time(NULL); //used to detect timeout
    time_t current_time = time(NULL);
    bool has_timeout = false; //flag for timeout
    bool logged_already = false;
    sensor_id_t id = 0; // Saving this id for later use

    do {

        start_time = time(NULL); // time before connection

        // Attempt to receive sensor ID
        bytes = sizeof(data.id);
        result = tcp_receive(client, (void*)&data.id, &bytes);
        if (result == TCP_NO_ERROR) {
        } else {
            break;
        }

        //on first data read, id of client must be logged:
        if (!logged_already) {
            //log id
            logged_already = true;
            log_sensor_connection(data.id);
            id = data.id;
        }

        // Attempt to receive temperature
        bytes = sizeof(data.value);
        result = tcp_receive(client, (void*)&data.value, &bytes);
        if (result == TCP_NO_ERROR) {
        } else {
            break;
        }

        // Attempt to receive timestamp
        bytes = sizeof(data.ts);
        result = tcp_receive(client, (void*)&data.ts, &bytes);
        if (result == TCP_NO_ERROR) {
        } else {
            break;
        }

        current_time = time(NULL);

        // Check if the timeout is reached
        if (difftime(current_time, start_time) > TIMEOUT) { // TIMEOUT in seconds
            has_timeout = true;
            break;
        }

        if (has_timeout) {
            break;
        }
        else {
            // Insert the received data into the shared buffer and log
            sbuffer_insert(&data);
            log_data_insert(id);
        }

    } while (result == TCP_NO_ERROR);

    if (result == TCP_CONNECTION_CLOSED){
        log_sensor_termination(id);
    }
    else if (has_timeout) {
        log_sensor_timeout(id);
    }
    else{
        write_to_log_process("Peer connection error\n");
    }

    // Close client socket
    tcp_close(&client);

    pthread_mutex_lock(&conn_mutex);
    conn_terminations++;

    // Signal the main thread if all connections are terminated
    if (conn_terminations == conn_counter) {
        pthread_cond_signal(&conn_cond);
    }
    pthread_mutex_unlock(&conn_mutex);

    return NULL;
}



bool stop_excess_handler = false;


//the method below handles and closes client connections once max_conn is reached
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
                write_to_log_process("Rejecting excess client connection\n");
                if (client) {
                    tcp_close(&client);
                    client = NULL;
                }
                write_to_log_process("Closed excess client socket\n");
            }
            pthread_mutex_unlock(&conn_mutex);
        } else if (client) {
            tcp_close(&client);
            client = NULL;
        }
    }
    write_to_log_process("Excess client handler terminating\n");
    return NULL;
}




void * connmgr_listen(void* arg){

  	connmgr_args_t* args = (connmgr_args_t*)arg; // Cast to connmgr_args_t*
    int PORT = args->port;
    MAX_CONN = args->max_conn;

    tcpsock_t *server, *client;
    pthread_t thread_id;

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

        write_to_log_process("Incoming client connection\n");

        pthread_mutex_lock(&conn_mutex);
        if (conn_counter < MAX_CONN) {
            conn_counter++;
            pthread_mutex_unlock(&conn_mutex);

            // Create a thread for the client
            if (pthread_create(&thread_id, NULL, client_handler, (void*)client) != 0) {
                perror("Failed to create thread");
                write_to_log_process("failed to create client thread\n");
                tcp_close(&client);
                continue;
            }
            pthread_detach(thread_id); // Detach the thread
        } else { //this else statement is never reached because while loop terminates before this happens
            pthread_mutex_unlock(&conn_mutex);
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
    stop_excess_handler = true;

    stop_excess_handler = true;
    pthread_join(excess_thread, NULL);

    write_to_log_process("Test server is shutting down\n");

    //insert eof
    sensor_data_t data = {0};  // Create an end-of-stream marker
	sbuffer_insert(&data); //insert end of stream to buffer
    free(args);
    return NULL;
}