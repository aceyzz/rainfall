#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int m; 

void p(char *str)
{
    printf(str); // vulnerabilité
}

void n(void)
{
    char buffer[512]; 
    
    fgets(buffer, 512, stdin);  // Lit jusqu'a 512 caractères depuis stdin
    p(buffer); // Appelle p() avec notre input
    
    if (m == 16930116) // Compare m avec 0x1025544 (16930116 en decimal)
    {
        system("/bin/cat /home/user/level5/.pass"); // Affiche le flag
    }
}

int main(void)
{
    n();
    return 0;
}