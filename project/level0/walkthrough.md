# Level 0

## Étape 1 : Observer le comportement du programme

### Première exécution sans argument
```bash
level0@RainFall:~$ ./level0
Segmentation fault (core dumped)
```
- Le programme attend un argument.

### Exécution avec un argument quelconque
```bash
level0@RainFall:~$ ./level0 test
No !
level0@RainFall:~$ ./level0 123
No !
level0@RainFall:~$ ./level0 hello
No !
```
- Le programme vérifie quelque chose et rejette nos entrées. Il attend probablement une valeur spécifique

## Étape 2 : Examiner les permissions du fichier

```bash
level0@RainFall:~$ ls -l level0
-rwsr-x---+ 1 level1 users 747441 Mar  6  2016 level0
```

- Le "s" dans "-rwsr-x---" = bit SUID activé
- Propriétaire : "level1" (pas level0 !)
- Cela signifie : quand on exécute ce programme, il tourne avec les privilèges de "level1"

- Si on trouve le bon input, on pourrait obtenir un shell avec les privilèges de level1.

## Étape 3 : Désassembler le programme avec GDB

```bash
level0@RainFall:~$ gdb level0
(gdb) disass main
```

### Ce qu'on voit (version simplifiée)
```bash
0x08048ec9 <+9>:   mov    0xc(%ebp),%eax     
0x08048ecc <+12>:  add    $0x4,%eax          
0x08048ecf <+15>:  mov    (%eax),%eax        
0x08048ed1 <+17>:  mov    %eax,(%esp)        
0x08048ed4 <+20>:  call   0x8049710 <atoi>   # Appel à atoi 
0x08048ed9 <+25>:  cmp    $0x1a7,%eax        # Comparaison 
0x08048ede <+30>:  jne    0x8048f58          # Saut si différent
```

### Analyse:

### [1] Les lignes +9 à +17 : Préparation de l'argument
```bash
mov    0xc(%ebp),%eax     # Récupère argv (tableau des arguments)
add    $0x4,%eax          # Saute argv[0], pointe vers argv[1]
mov    (%eax),%eax        # Charge le contenu de argv[1]
mov    %eax,(%esp)        # Place argv[1] sur la pile
```
- Le programme prépare argv[1] pour le passer à une fonction.

### [2] Ligne +20 : Appel à atoi
```bash
call   0x8049710 <atoi>
```
- atoi(): Convertit une chaîne de caractères en entier.
	Input : "123" → Output : 123
	Input : "test" → Output : 0

- Notre argument est converti en nombre et stocké dans "%eax"

### [3] Ligne +25 : La comparaison
```bash
cmp    $0x1a7,%eax
```
Explications:
- cmp = compare deux valeurs
- $0x1a7 = valeur hexadécimale immédiate
- %eax = résultat de atoi (notre input converti)

### [4] Ligne +30 : Saut conditionnel
```bash
jne    0x8048f58
```
- jne = jump if not equal
- Si %eax != 0x1a7 → saute vers 0x8048f58

- Ce qu'il y a à 0x8048f58:
```bash
0x08048f58 <+152>:   mov    0x80ee170,%eax
[...]
0x08048f7b <+187>:   call   0x804a230 <fwrite>    # Affiche qq chose
```

## Étape 4: Identifier la valeur secrète

- On sait que le programme compare notre input avec 0x1a7

### Conversion hexadécimal vers décimal

- Avec python:
	```bash
	python -c "print(0x1a7)"
	423
	```

- Le programme attend donc l'argument 423

## Étape 5: Ce qui se passe si la comparaison reussit

- Si %eax == 0x1a7, le "jne" ne saute pas, donc le programme continue:

```bash
0x08048ee0 <+32>:  movl   $0x80c5348,(%esp)
0x08048ee7 <+39>:  call   0x8050bf0 <strdup>
```

- Que contient l'adresse 0x80c5348:
```bash
(gdb) x/s 0x80c5348
0x80c5348:   "/bin/sh"
```
- Le programme prépare /bin/sh

### Suite du code
```bash
0x08048ef8 <+56>:  call   0x8054680 <getegid>    # Récupère GID effectif
0x08048f01 <+65>:  call   0x8054670 <geteuid>    # Récupère UID effectif
[...]
0x08048f21 <+97>:  call   0x8054700 <setresgid>  # Définit les GID
0x08048f3d <+125>: call   0x8054690 <setresuid>  # Définit les UID
0x08048f51 <+145>: call   0x8054640 <execv>      # Lance /bin/sh
```

Explications :
1. Récupère les IDs effectifs (ceux de level1 grâce au SUID)
2. Définit tous les IDs (réel, effectif, sauvegardé) à ces valeurs
3. Lance un shell `/bin/sh` qui hérite de ces privilèges

## Étape 7 : Exploitation finale

```bash
level0@RainFall:~$ ./level0 423
$ whoami
level1
$ cat /home/user/level1/.pass
1fe8a524fa4bec01ca4ea2a869af2a02260d4a7d5fe7e7c24d8617e6dca12d3a
```
