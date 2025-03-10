#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include "main.h"
#include "../shared.h"

char* homeDir;
Connection connections[MAX_CONNECTIONS];

void PrintErrorMessage(char* message) {
    printf("\033[1;31m%s\033[0m", message);
}

void PrintSuccessMessage(char* message) {
    printf("\033[1;32m%s\033[0m", message);
}

void SendMessageToClient(Connection* connection, uint8_t type, char* message) {
    uint8_t buffer[1024];
    buffer[0] = type;
    int message_len = strlen(message);
    memcpy(&buffer[1], message, message_len + 1);
    int sentMessageToClient = send(connection->clientfd, buffer, message_len + 2, 0);

    if(sentMessageToClient < 0) {
        PrintErrorMessage("Error sending a message to client\n");
    } else {
        PrintSuccessMessage("Successfully sent a message to client\n");
    }
}

void createTable(Connection* connection) {
    char* tableName = connection->tokens[2];

    if(connection->connectedDatabase == NULL) {
        SendMessageToClient(connection, PRINT_ERROR, "Please connect to a database before creating a table\n");
        return;
    }

    char file_location[1024];
    snprintf(file_location, sizeof(file_location), "%s/.bytedb/%s/%s.bdb", homeDir, connection->connectedDatabase, tableName);

    printf("file location: %s table name: %s connected database: %s\n", file_location, connection->tokens[2], connection->connectedDatabase);

    if(access(file_location, R_OK) != -1) {
        SendMessageToClient(connection, PRINT_ERROR, "Table with this name already exists\n");
        return;
    }

    FILE *tableFile = fopen(file_location, "w");
    if(tableFile == NULL) {
        SendMessageToClient(connection, PRINT_ERROR, "Failed to create a file for the table\n");
        return;
    }

    fclose(tableFile);

    SendMessageToClient(connection, PRINT_SUCCESS, "Successfully created a new table\n");
}

void connectDatabase(Connection* connection) {
    char* databaseName = connection->tokens[2];

    if(databaseName == NULL) {
        return;
    };

    char databaseDirBuffer[1024];

    snprintf(databaseDirBuffer, sizeof(databaseDirBuffer), "%s/.bytedb/%s", homeDir, databaseName);

    DIR *databaseDir = opendir(databaseDirBuffer);
    if(!databaseDir) {
        return;
    }

    size_t databaseNameLength = strlen(databaseName) + 1;

    char* databaseNameClone = malloc(databaseNameLength);

    strncpy(databaseNameClone, databaseName, databaseNameLength);

    connection->connectedDatabase = databaseNameClone;

    SendMessageToClient(connection, PRINT_SUCCESS, "Successfully connected to database\n");
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
        PrintErrorMessage("Error creating a directory for bytedb data!!\n");
        exit(1);
    }
}

Server CreateTcpServer() {
    int sockfd;
    struct sockaddr_in address;
    int addr_len = sizeof(address);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == 0) {
        PrintErrorMessage("Creating socket failed!!\n");
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    int bindedSocket = bind(sockfd, (struct sockaddr *)&address, addr_len);

    if(bindedSocket < 0) {
        PrintErrorMessage("Binding socket failed!!\n");
        exit(1);
    }

    PrintSuccessMessage("Successfully binded socket\n");

    int startedListening = listen(sockfd, 3);

    if(startedListening < 0) {
       PrintErrorMessage ("Error listening on socket!!\n");
        exit(1);
    }

    PrintSuccessMessage("Successfully Started Listening\n");

    Server server = (Server){sockfd, address, addr_len};

    return server;
}

Command commands[] = {
    {"database create", createDatabase},
    {"database connect", connectDatabase},
    {"table create", createTable},
};

void* handleClient(void* arg) {
    Connection* connection = (Connection *)arg;

    connection->tokens = calloc(MAX_TOKENS, sizeof(char*));
    if(connection->tokens == NULL) {
        SendMessageToClient(connection, PRINT_ERROR, "Failed to allocate memory for tokens\n");
        return NULL;
    }

    while(1) {
        if(connection->tokens != NULL) {
            for(unsigned int i = 0; i < MAX_TOKENS; i++) {
                if(connection->tokens[i] != NULL) {
                    free(connection->tokens[i]);
                    connection->tokens[i] = NULL;
                    continue;
                }
                
                break;
            }
        }

        char buffer[4096] = {0};
        
        int readFromClient = read(connection->clientfd, buffer, sizeof(buffer));

        if(readFromClient == 0) {
            PrintErrorMessage("Connection closed by client\n");
            break;
        } else if(readFromClient < 0) {
            PrintErrorMessage("Reading from client failed\n");
            continue;
        }

        char buffer_clone[sizeof(buffer)] = {0};
        strncpy(buffer_clone, buffer, strlen(buffer));

        char* token = strtok(buffer_clone, TOKEN_DELIMETERS);

        for(unsigned int i = 0; token != NULL; i++) {
            size_t token_len = strlen(token) + 1;

            char* token_copy = malloc(token_len);

            strncpy(token_copy, token, token_len);

            printf("tkn copy: %s tkn: %s\n", token_copy, token);

            connection->tokens[i] = token_copy;
        
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
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        clientfd = accept(server.sockfd, (struct sockaddr *)&client_addr, &addr_len);

        if(clientfd < 0) {
            PrintErrorMessage("Failed to accept request\n");
            continue;
        }

        Connection* connection = getEmptyConnection();

        connection->tokens = NULL;
        connection->clientfd = clientfd;
        connection->addr_len = addr_len;
        connection->client_addr = client_addr;

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
