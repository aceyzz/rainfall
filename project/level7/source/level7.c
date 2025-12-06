#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ------------------------ Structures ------------------------

struct _IO_FILE {
    int32_t e0;
};

// --------------------- Global Variables ---------------------

char * g1; // 0x8049960

// ------------------------ Functions -------------------------

// Address range: 0x8048521 - 0x8048603
int main(int argc, char ** argv) {
    int32_t * mem = malloc(8); // 0x8048531
    *mem = 1;
    int32_t * mem2 = malloc(8); // 0x804854b
    int32_t * str = (int32_t *)((int32_t)mem + 4); // 0x8048556
    *str = (int32_t)mem2;
    int32_t * mem3 = malloc(8); // 0x8048560
    *mem3 = 2;
    int32_t * mem4 = malloc(8); // 0x804857a
    int32_t * str2 = (int32_t *)((int32_t)mem3 + 4); // 0x8048585
    *str2 = (int32_t)mem4;
    strcpy((char *)*str, (char *)*(int32_t *)(argc + 4));
    strcpy((char *)*str2, (char *)*(int32_t *)(argc + 8));
    fgets((char *)&g1, 68, fopen("/home/user/level8/.pass", "r"));
    puts("~~");
    return 0;
}