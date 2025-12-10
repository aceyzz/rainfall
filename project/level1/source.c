#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// ft jamais appelée
void run(void)
{
    fwrite("Good... Wait what?\n", 1, 19, stdout);
    system("/bin/sh");
    return;
}

int main(void)
{
    // buff 76 octets: vuln au buff overflow
    char buffer[76];
    
    // gets() ne vérifie pas la taille
    gets(buffer);
    
    return 0;
}