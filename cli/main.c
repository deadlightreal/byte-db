#include <stdlib.h>
#include <stdint.h>
#include "../shared.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>

Server server = {-1};
char *homeDir;
char input[1024];

void PrintCursor() {
    printf("\033[1G\033[1;36m >  \033[0m");
}

void quitDatabase() {
    system("clear");
    exit(0);
}

void clearScreen() {
    system("clear");
}

void PrintErrorMessage(char* message) {
    printf("\033[1G\033[1;31m%s\033[0m", message);
    PrintCursor();
}

void PrintSuccessMessage(char* message) {
    printf("\033[1G\033[1;32m%s\033[0m", message);
    PrintCursor();
}

void SendCommandToServer() {
    int data_sent = send(server.sock, input, strlen(input), 0);
    if(data_sent == -1) {
        PrintErrorMessage("Error sending command to server\n");
    } else {
        PrintSuccessMessage("Successfully executed command\n");
    }
}

void connectToServer() {
    server.sock = socket(AF_INET, SOCK_STREAM, 0);

    if(server.sock < 0) {
        PrintErrorMessage("Failed to create a socket!!\n");
        exit(EXIT_FAILURE);
    }

    server.server_addr.sin_family = AF_INET;
    server.server_addr.sin_port = htons(PORT);
    if(inet_pton(AF_INET, SERVER_IP, &server.server_addr.sin_addr) <= 0) {
        PrintErrorMessage("Invalid address or address not supported\n");
        exit(EXIT_FAILURE);
    }

    int connectedToServer = connect(server.sock, (struct sockaddr *)&server.server_addr, sizeof(server.server_addr));
    
    if(connectedToServer < 0) {
        PrintErrorMessage("Error connecting to server\n");
        exit(EXIT_FAILURE);
    }

    PrintSuccessMessage("Successfully connected to server\n");
}

void* handleReadingFromServer(void* sockfd_arg) {
    int sockfd = *(int *)sockfd_arg;

    uint8_t buffer[1024];

    PrintSuccessMessage("Successfully running thread\n");

    while(1) {
        int readFromServer = recv(sockfd, buffer, sizeof(buffer), 0);
        if(readFromServer == 0) {
            PrintErrorMessage("Connection closed by server\n");
            exit(EXIT_FAILURE);
        } else if(readFromServer < 0) {
            PrintErrorMessage("Reading from server failed\n");
            continue;
        }

        int type = buffer[0];
        char* message = (char*)&buffer[1];

        switch(type) {
            case PRINT_SUCCESS:
                PrintSuccessMessage(message);
                break;
            case PRINT_ERROR:
                PrintErrorMessage(message);
                break;
        }
    }

    return NULL;
}

int main() {
    system("clear");

    printf("\n\n\n\033[1;36m"
"  ______     _________ ______     _____  ____  \n"
" |  _ \\ \\   / /__   __|  ____|   |  __ \\|  _ \\ \n"
" | |_) \\ \\_/ /   | |  | |__      | |  | | |_) |\n"
" |  _ < \\   /    | |  |  __|     | |  | |  _ < \n"
" | |_) | | |     | |  | |____    | |__| | |_) |\n"
" |____/  |_|     |_|  |______|   |_____/|____/ \n"
"\033[0m\n\n\n");

    connectToServer();

    Command commands[] = {
        {"quit", quitDatabase},
        {"clear", clearScreen},
    };

    pthread_t serverReadingThread;
    
    int created_thread = pthread_create(&serverReadingThread, NULL, handleReadingFromServer, (void *)&server.sock);

    if(created_thread != 0) {
        PrintErrorMessage("Failed to create a thread to read messages from server\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        PrintCursor();
        
        fgets(input, sizeof(input), stdin);
        
        input[strcspn(input, "\n")] = 0;

        bool executedCommand = false;

        for(uint8_t i = 0; i < sizeof(commands) / sizeof(Command); i++) {
            Command current_command = commands[i];

            if(strncmp(input, current_command.command, strlen(current_command.command)) == 0) {
                current_command.function();

                executedCommand = true;
            }
        }

        if(executedCommand == false) {
            SendCommandToServer();
        }
    }
    
    return 0;
}
