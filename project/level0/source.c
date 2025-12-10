#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    int input;
    char *shell_args[2];
    gid_t gid;
    uid_t uid;
    
    // Convertit l'argument en entier
    input = atoi(argv[1]);
    
    // Vérifie si l'entrée correspond à 423
    if (input == 423) {
        // Prépare les arguments pour execv
        shell_args[0] = strdup("/bin/sh");
        shell_args[1] = NULL;
        
        // Récupère les IDs effectifs
        gid = getegid();
        uid = geteuid();
        
        // Définit les privilèges
        setresgid(gid, gid, gid);
        setresuid(uid, uid, uid);
        
        // Lance le shell
        execv("/bin/sh", shell_args);
    } else {
        // Affiche le message d'erreur
        fwrite("No !\n", 1, 5, stderr);
    }
    
    return 0;
}