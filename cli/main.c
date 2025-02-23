#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include <arpa/inet.h>
#include <sys/socket.h>

Server server = {-1};
char *homeDir;
char input[1024];

void quitDatabase() {
    system("clear");
    exit(0);
}

void clearScreen() {
    system("clear");
}

void PrintErrorMessage(char* message) {
    printf("\033[1;31m%s\033[0m", message);
}

void PrintSuccessMessage(char* message) {
    printf("\033[1;32m%s\033[0m", message);
}

void SendCommandToServer() {
    printf("input: %s\n", input);
    printf("strlen: %lu\n", strlen(input));
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
        PrintErrorMessage("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    int connectedToServer = connect(server.sock, (struct sockaddr *)&server.server_addr, sizeof(server.server_addr));
    
    if(connectedToServer < 0) {
        PrintErrorMessage("Error connecting to server\n");
        exit(EXIT_FAILURE);
    }

    PrintSuccessMessage("Successfully connected to server\n");
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
        {"create database", SendCommandToServer}
    };

    while (1) {
        printf("\033[1;36m\n>  \033[0m");
        
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

        printf("\n");

        if(executedCommand == false) {
            PrintErrorMessage("Uknown command!!\n");
        }
    }
    
    return 0;
}
