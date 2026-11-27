# Deep Sweep — Prototype jouable (MVP)

Maquette navigateur pour **valider le fun** avant le portage sur Unreal Engine (voir `../GDD.md` §15).

## Lancer
Double-cliquer sur `index.html` (Chrome, Edge ou Firefox). Aucune installation, fonctionne hors ligne.

## Joueurs (1 à 4, même écran)
| Joueur | Bouger | Attraper / interagir | Action |
|---|---|---|---|
| Clavier 1 | ZQSD / WASD | F | G |
| Clavier 2 | Flèches | Entrée | Maj droite |
| Manettes | Stick gauche | A | X ou B |

Dans le menu, chaque joueur appuie sur **attraper** pour rejoindre, puis **Espace** / **Start**.

## Contenu
- Crique de ~150 déchets (6 types) en 6 tas, courant marin, colonne de bulles, objets qui flottent ou coulent.
- **Pince** : ramasser, lancer (on peut viser le comptoir ou le tapis).
- **Aspirateur à 2 postes** : buse (attraper puis action pour allumer) + console de puissance. À puissance 4–5, il aspire les collègues et casse les vases. Les caddies bouchent le tuyau (action ×3).
- **Cuve + levier + convoyeur** vers le comptoir ; les caddies bloquent le tapis (attraper pour débloquer).
- **Objets géants coop** : coffre (2 porteurs) et statue (3 porteurs).
- **5 améliorations** sur les pastilles violettes de la base.
- **Poulpe voleur** (événement) : vole jusqu'à 3 objets précieux ; l'attraper le fait tout lâcher (+20₡), sinon il cache son butin près du bord. Plus fréquent à plusieurs.
- **Sons** générés (combo de vente qui monte en pitch, aspiration, bouchons…).
- **Objectif** : 100 % de propreté → la crique reprend ses couleurs, écran de fin avec stats, **R** / **Start** pour rejouer.

## Fun de 1 à 4 joueurs
- **Bob le robot** occupe les postes vides : la console reste à puissance 2 et il vide la cuve automatiquement (plus lentement qu'un humain).
- **Objets géants** : nombre de porteurs requis plafonné au nombre de joueurs, mais vitesse bonifiée quand l'équipe est complète.
- Caméra partagée qui dézoome pour garder tout le monde à l'écran.
