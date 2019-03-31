#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "struct.h"
#include <string.h>


//initialize instructions
INST instruction[13] = {
        {.instruction = "Not Left", .answer = 1},
        {.instruction = "Not Right", .answer = 0},
        {.instruction = "Not Not up", .answer = 2},
        {.instruction = "Not Down", .answer = 2},
        {.instruction = "Not Not Down", .answer = 3},
        {.instruction = "Not Up", .answer = 3},//
        {.instruction = "Not Not Left", .answer = 0},
        {.instruction = "Not Not Right", .answer = 1},
        {.instruction = "Not Not Not Left", .answer = 1},
        {.instruction = "Not Not Not Right", .answer = 0},
        {.instruction = "Not Not Not Up", .answer = 3},
        {.instruction = "Not Not Not Down", .answer = 2},
};




int main() {
    printf("Hello, World!\n");
    return 0;
}
