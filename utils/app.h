#ifndef _APP_H_
#define _APP_H_


#include <stdio.h>
#include <signal.h>


#include "socket.h"
#include "log.h"
#include "md5.h"
#include "os.h"


#define MAX_RETRY 16
#define TIMEOUT_MS 10
#define WINDOW_SIZE 32
#define BUFFER_SIZE 1024


typedef struct __attribute__((packed)) {
    // The maximum length of single UDP packet is `65507` bytes (note: `unsigned short` is `65536` bytes).
    unsigned int length;
    unsigned int sequence;
    char data[BUFFER_SIZE];
} PacketUDP;


typedef struct {
    // Type `acknowledge` must be consistent with type `PacketUDP.length`.
    unsigned int acknowledge;
    unsigned int window_size;
} PacketACK;


typedef struct {
    long second;
    int microsecond;
} Time;


void app_interrupt(int semaphore);


int app_handshake(Socket c, struct sockaddr_in *server, int server_size);


int app_server(int port);


int app_client(char *app_name, char *filepath, char *ipv4, int port);


void app_convert(unsigned int value, char *buffer, int size);


void app_progress(int current, int epoch, int step, char *description);


int app_parse(char *address, char *ipv4, int *port);


void app_write_config(char *ip, int port);


int app_read_config(char *ip, int *port);


#endif