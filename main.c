#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

int currentToken = 0;
char** tokens = NULL;

void quitDatabase() {
    system("clear");
    exit(0);
}

char* getNextToken() {
    if(currentToken >= MAX_TOKENS) {
        return NULL;
    }

    char* token = tokens[currentToken];

    currentToken++;

    return token;
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

    char input[500];

    Command commands[] = {
        {"quit", quitDatabase}
    };

    tokens = (char**)calloc(MAX_TOKENS, sizeof(char*));

    if(tokens == NULL) {
        fprintf(stderr, "Failed to execute calloc\n");
        exit(1);
    }

    while (1) {
        printf("\033[1;36m> \033[0m");
        
        fgets(input, sizeof(input), stdin);
        
        input[strcspn(input, "\n")] = 0;

        char* current_token = strtok(input, TOKENIZING_DELIM);

        memset(tokens, 0, MAX_TOKENS * sizeof(char*));

        for(uint8_t i = 0; ; i++) {
            tokens[i] = current_token;

            current_token = strtok(NULL, TOKENIZING_DELIM);

            if(current_token == NULL) {
                break;
            }
        }

        char* command = getNextToken();

        for(uint8_t i = 0; i < sizeof(commands) / sizeof(Command); i++) {
            if(command == NULL) break;

            Command current_command = commands[i];

            if(strncmp(command, current_command.input, sizeof(input)) == 0) {
                current_command.function();
            }
        }

        currentToken = 0;
    }
    
    return 0;
}
