#pragma once

#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

typedef struct {
    int sockfd;
    struct sockaddr_in address;
    unsigned int addr_len;
} Server;

typedef struct {
    bool occupied;
    pthread_t thread;
    int clientfd;
    char** tokens;
} Connection;

typedef struct {
    const char* command;
    void (*function)(Connection*);
} Command;


#define TOKEN_DELIMETERS " "
#define MAX_TOKENS 500
#define PORT 10024
#define MAX_CONNECTIONS 10
