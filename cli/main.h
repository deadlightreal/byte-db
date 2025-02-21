#pragma once

#include <arpa/inet.h>

#define TOKENIZING_DELIM " "
#define MAX_TOKENS 100

#define SERVER_IP "127.0.0.1"
#define PORT 10024

typedef struct {
    const char* command;
    void (*function)(void);
} Command;

typedef struct {
    int sock;
    struct sockaddr_in server_addr;
} Server;
