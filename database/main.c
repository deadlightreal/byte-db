#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include "main.h"

char* homeDir;
Connection connections[MAX_CONNECTIONS];

void PrintErrorMessage(char* message) {
    printf("\033[1;31m%s\033[0m", message);
}

void PrintSuccessMessage(char* message) {
    printf("\033[1;32m%s\033[0m", message);
}

void createDatabase(Connection* connection) {
    char* databaseName = connection->tokens[2];

    if(databaseName == NULL) {
        printf("Please provide a database name\n");
        return;
    }

    char databaseDirBuffer[1024];

    snprintf(databaseDirBuffer, sizeof(databaseDirBuffer), "%s/.bytedb/%s", homeDir, databaseName);

    DIR *databaseDir = opendir(databaseDirBuffer);
    if(databaseDir) {
        printf("database with name %s already exists!!\n", databaseName);
        return;
    }

    int createdDatabaseDir = mkdir(databaseDirBuffer, 0777);
    if(createdDatabaseDir != 0) {
        printf("failed to create a database\n");
        return;
    }

    PrintSuccessMessage("Successfully created a new database\n");
};

void initializeBytedbDirectory() {
    char bytedbDirectory[1024];

    snprintf(bytedbDirectory, sizeof(bytedbDirectory), "%s/.bytedb", homeDir);

    DIR *bytedbDir = opendir(bytedbDirectory);
 
    if(bytedbDir) return;

    int createdDirectory = mkdir(bytedbDirectory, 0777);
    if(createdDirectory != 0) {
        printf("Error creating a directory for bytedb data!!\n");
        exit(1);
    }
}

Server CreateTcpServer() {
    int sockfd;
    struct sockaddr_in address;
    int addr_len = sizeof(address);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == 0) {
        printf("Creating socket failed!!\n");
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    int bindedSocket = bind(sockfd, (struct sockaddr *)&address, addr_len);

    if(bindedSocket < 0) {
        printf("Binding socket failed!!\n");
        exit(1);
    }

    PrintSuccessMessage("Successfully binded socket\n");

    int startedListening = listen(sockfd, 3);

    if(startedListening < 0) {
        printf("Error listening on socket!!\n");
        exit(1);
    }

    PrintSuccessMessage("Successfully Started Listening\n");

    Server server = (Server){sockfd, address, addr_len};

    return server;
}

Command commands[] = {
    {"create database", createDatabase}
};

void* handleClient(void* arg) {
    Connection* connection = (Connection *)arg;

    while(1) {
        if(connection->tokens != NULL) {
            free(connection->tokens);
            connection->tokens = NULL;
        }

        connection->tokens = calloc(MAX_TOKENS, sizeof(char*));

        char buffer[4096];

        int readFromClient = read(connection->clientfd, buffer, sizeof(buffer));

        if(readFromClient == 0) {
            PrintErrorMessage("Connection closed by client\n");
            break;
        } else if(readFromClient < 0) {
            PrintErrorMessage("Reading from client failed\n");
            continue;
        }

        char buffer_clone[sizeof(buffer)];
        strncpy(buffer_clone, buffer, strlen(buffer));

        char *token = strtok(buffer_clone, TOKEN_DELIMETERS);

        for(unsigned int i = 0; token != NULL; i++) {
            connection->tokens[i] = token;
        
            token = strtok(NULL, TOKEN_DELIMETERS);
        }

        for(unsigned int i = 0; i < sizeof(commands) / sizeof(Command); i++) {
            Command command = commands[i];
            if(strncmp(command.command, buffer, strlen(command.command)) == 0) {
                command.function(connection);
            }
        }

        PrintSuccessMessage("Successfully read from client\n");
    }

    return NULL;
}

Connection* getEmptyConnection() {
    for(unsigned int i = 0; i < MAX_CONNECTIONS; i++) {
        Connection con = connections[i];

        if(con.occupied == false) {
            con.occupied = true;

            return &connections[i];
        }
    }

    return NULL;
}

void HandleConnections(Server server) {
    while(1) {
        int clientfd;

        clientfd = accept(server.sockfd, (struct sockaddr *)&server.address, &server.addr_len);

        if(clientfd < 0) {
            PrintErrorMessage("Failed to accept request\n");
            continue;
        }

        Connection* connection = getEmptyConnection();

        connection->tokens = NULL;
        connection->clientfd = clientfd;

        if(connection == NULL) {
            PrintErrorMessage("Failed to get empty connection\n");
            continue;
        }

        int createdThread = pthread_create(&connection->thread, NULL, handleClient, (void *)connection);

        if(createdThread != 0) {
            PrintErrorMessage("Failed to create a thread to handle client connection\n");
            continue;
        }
    }
}

int main() {
    homeDir = getenv("HOME");

    initializeBytedbDirectory();

    Server server = CreateTcpServer();

    memset(connections, 0, sizeof(connections));

    HandleConnections(server);

    return 0;
}
