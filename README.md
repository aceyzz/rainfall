<img title="42_RAINFALL" alt="42_RAINFALL" src="./utils/banner.png" width="100%">

<br>

# RAINFALL — 42

Projet d’introduction à l’exploitation de binaires ELF en environnement i386. L’objectif est d’enchaîner des niveaux successifs en exploitant des vulnérabilités réelles (buffer overflows, erreurs de logique, mauvaises pratiques mémoire) pour récupérer les mots de passe et progresser jusqu’aux comptes bonus.  

<br>

## Description

Parcours complet de 10 niveaux d’exploitation sur VM : analyse de binaires, compréhension fine de la stack, étude du comportement mémoire, construction d’exploits reproductibles et justification de chaque étape. Chaque niveau impose d’obtenir le .pass du niveau suivant via une faille exploitable, sans brute force ni automatisation. Le dépôt contient pour chaque niveau : source reconstitué, walkthrough précis, ressources nécessaires et explications détaillées. Projet centré sur la rigueur, la méthode d’analyse et la capacité à comprendre et détourner un binaire vulnérable, en respectant strictement les règles de sécurité et de validation.  

<br>

## Index des niveaux

- [Level 5](./project/level5) - `Format string vulnerability` menant à l’exécution non prévue de code  
- [Level 6](./project/level6) - `Heap buffer overflow` menant à la corruption d’un pointeur de fonction et à l’exécution non prévue de code  
- [Level 7](./project/level7) - `Heap buffer overflow` menant au détournement d’une entrée GOT et à l’exécution non prévue de code  
- [Level 8](./project/level8) - `Heap exploitation via use-after-free / heap feng shui` menant à la falsification d’un pointeur interne et à l’exécution non prévue de code
- [Level 9](./project/level9) - `Heap overflow sur objets C++` permettant l’écrasement de la vtable et l’exécution d’un shellcode

<br>

## Liens utiles

- [Sujet officiel](./utils/en.subject.pdf)

<br>

## Grade

> En cours de construction

<!-- <img src="./utils/100.png" alt="Grade" width="150"> -->

<br>

## Authors

<table>
	<tr>
		<td align="center">
			<a href="https://github.com/cduffaut" target="_blank">
				<img src="https://avatars.githubusercontent.com/u/119802570?v=4" width="40" alt="GitHub Icon"/><br>
				<b>Cécile</b>
			</a>
		</td>
		<td align="center">
			<a href="https://github.com/aceyzz" target="_blank">
				<img src="https://avatars.githubusercontent.com/u/112768475?v=4" width="40" alt="GitHub Icon"/><br>
				<b>Cédric</b>
			</a>
		</td>
	</tr>
</table>
