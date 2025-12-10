#include <stdio.h>
#include <stdlib.h>

int m = 0;

void v(void)
{
    char buffer[520];  
    
    fgets(buffer, 512, stdin);
    
    // vuln: printf sans format fixe
    printf(buffer);
    
    // check si m == 64 (0x40)
    if (m == 64)
    {
        fwrite("Wait what?!\n", 1, 12, stdout);
        system("/bin/sh");
    }
}

int main(void)
{
    v();
    return 0;
}