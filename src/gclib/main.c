#include <string.h>
#include <stdio.h>

// #include "args.h"
#include "fs.h"

int main(int argc, char* args[]) {
    if (argc != 2) {
        printf("Argument mismatch\n");
        return 1;
    }

    char* path;
    // strcpy(path, args[1]);

    listdir(args[1]);
    return 0;
}