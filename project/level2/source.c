#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void p(void)
{
    unsigned int return_addr;
    char buffer[76];
    
    fflush(stdout);
    
    gets(buffer);
    
    // Récupère l'adresse de retour depuis la stack
    // En assembleur: mov 0x4(%ebp),%eax
    return_addr = __builtin_return_address(0);
    
    // Vérifie si l'adresse de retour pointe vers la stack (commence par 0xb)
    if ((return_addr & 0xb0000000) == 0xb0000000) {
        printf("(%p)\n", (void *)return_addr);
        _exit(1);
    }
    
    puts(buffer);
    strdup(buffer);
}

int main(void)
{
    p();
    return 0;
}