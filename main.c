#include "std.h"
#include "bat.h"
#include "run.h"
#include <shlwapi.h>
#include <string.h>
#include <stdlib.h>

int main() {
    system("cls");
    // print information
    printf("tiny-shell v1.0\n");
    printf("HoangNV, PhucVH\n");
    printf("20182 Dai hoc Bach khoa Ha Noi\n");
    
    char inputString[MAX_PATH];

    while (1) {
        //Print directory and prompt
        PrintCurrentDirectory();
        //Read command
        ReadCommand(inputString, MAX_PATH);
        // Run command
        RunCommand(inputString);
    }
    return 0;
}