#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ------------------------ Functions -------------------------

// Address range: 0x804847c - 0x80484d4
int main(int argc, char ** argv) {
    int32_t * mem = malloc(64); // 0x804848c
    int32_t * mem2 = malloc(4); // 0x804849c
    *mem2 = 0x8048468;
    strcpy((char *)mem, (char *)*(int32_t *)(argc + 4));
    return *mem2;
}
