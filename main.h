#pragma once

#define TOKENIZING_DELIM " "
#define MAX_TOKENS 100

typedef struct {
    const char* input;
    void (*function)(void);
} Command;
