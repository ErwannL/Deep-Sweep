# Prompt à donner à Claude Code sur ton PC (portage Unreal Engine 5)

> À coller dans Claude Code (desktop app ou terminal), lancé **dans le dossier du dépôt Deep-Sweep cloné sur ton PC**.

---

Tu travailles sur mon PC Windows pour transformer le jeu **Deep Sweep** en vrai projet **Unreal Engine 5** en 3D.

## Contexte
- Le dépôt contient `GDD.md` (Game Design Document complet) et `prototype/` (maquette jouable navigateur en three.js qui a validé la boucle de jeu). **Lis les deux en entier avant de commencer.**
- Objectif : reproduire le MVP décrit dans `GDD.md` §15 (et ce que fait `prototype/index.html`) en vrai jeu UE5, avec de la vraie 3D, une vraie physique (Chaos) et du **multijoueur en ligne 1 à 4 joueurs** (listen server), fun quel que soit le nombre de joueurs (voir GDD §6.3 : Bob le robot, porteurs plafonnés, etc.).
- Architecture à respecter : GDD §14 (objets dormants/actifs/critiques, convoyeur logique, aspiration « virtualisée », sous-systèmes C++ `UDSPickupSubsystem`, `ADSMachine`, `UDSCarryComponent`, etc.).

## Avant de toucher à quoi que ce soit
1. Vérifie mon environnement et dis-moi ce qui manque : version d'Unreal Engine installée (chemin de `UnrealEditor.exe`), Visual Studio 2022 avec les workloads « Développement Desktop C++ » et « Développement de jeux C++ », Windows SDK, Git.
2. Propose-moi un **plan par étapes** et attends ma validation.
3. Demande-moi avant toute installation, tout téléchargement ou toute action en dehors du dossier du projet.

## Façon de travailler
- Crée le projet C++ dans `unreal/DeepSweep/` (template Blank C++, sans starter content).
- Privilégie ce que tu peux faire **sans cliquer** : écrire le C++ et les fichiers de config toi-même, compiler avec `UnrealBuildTool` / `Build.bat`, et générer niveaux, matériaux, Blueprints et Data Assets via des **scripts Python de l'éditeur** (active le plugin *Python Editor Script Plugin*) lancés avec `UnrealEditor-Cmd.exe ... -run=pythonscript -script=...`.
- N'utilise le contrôle de l'écran (computer use) que lorsque c'est vraiment nécessaire (ex. vérifier visuellement le rendu, lancer le jeu en PIE avec plusieurs joueurs). Fais des captures d'écran pour vérifier ton travail.
- Après chaque étape : compile, corrige les erreurs, lance le jeu, vérifie que ça marche, puis **commit** avec un message clair. Ne passe pas à l'étape suivante tant que l'actuelle ne fonctionne pas.
- Ajoute un `.gitignore` Unreal (Binaries, Intermediate, Saved, DerivedDataCache, .vs). Les gros assets binaires passent par Git LFS (demande-moi avant de l'activer).

## Étapes attendues (MVP)
1. **Projet + plongeur** : personnage en nage 3D arcade (Character Movement en mode Flying/Swimming custom), caméra à la troisième personne, Enhanced Input clavier + manette, flottement léger.
2. **Monde** : la crique (~80×80 m, 5–20 m de profondeur) avec fond sableux, rochers, coraux, brouillard volumétrique bleu, rayons de lumière, particules Niagara (bulles, sédiments), une zone de courant et une colonne de bulles.
3. **Objets** : 150 objets de 6 types (Data Assets : canette, bouteille, ferraille, pneu, caddie, vase) avec flottabilité (coulent/flottent), système dormant/actif, code couleur lisible + contour au survol.
4. **Pince** : attraper, porter (capacité), lancer.
5. **Comptoir de vente** + crédits partagés + % de propreté qui fait évoluer les couleurs du décor (sale → propre).
6. **Aspirateur industriel à 2 postes** (buse au bout d'un tuyau physique en segments + console de puissance 1–5), cuve, levier, bouchons de caddie, aspiration des coéquipiers à puissance ≥ 4, vases qui cassent.
7. **Convoyeur** logique (objets sur spline rendus en instanced mesh) entre la cuve et le comptoir, qui peut se bloquer.
8. **Objets géants coop** : coffre (2 porteurs) et statue (3 porteurs), nombre de porteurs plafonné au nombre de joueurs, plus rapide à plusieurs.
9. **5 améliorations** (sac, pince télescopique, turbo aspirateur, tapis rapide, ballons) achetables à la base.
10. **Bob le robot** qui tient les postes vides, **poulpe voleur** (événement), écran de fin avec stats drôles.
11. **Multijoueur** : listen server via Online Subsystem Steam (ou Null pour tester en LAN), menu héberger/rejoindre, test en PIE à 2 puis 4 clients avec latence simulée (`Net PktLag=100`).
12. **Juice** : sons, petits screen shakes, popups de crédits, musique qui s'intensifie quand la chaîne tourne.

## Direction artistique
3D stylisée cartoon, couleurs saturées, objets surdimensionnés et lisibles, plongeurs trapus à gros casque rond expressif (GDD §13). Commence avec des formes simples (meshes générés ou primitives) et des matériaux propres. Si tu as besoin d'assets 3D gratuits (Fab / Quixel / Kenney, licence CC0), **propose-les-moi avant de télécharger**.

## Livrables
- Un projet qui compile et se lance, jouable de 1 à 4 joueurs.
- Un `unreal/README.md` expliquant comment compiler, lancer, héberger et rejoindre une partie.
- À la fin de chaque session de travail, un résumé : ce qui marche, ce qui ne marche pas encore, prochaine étape.

Commence par l'étape « Avant de toucher à quoi que ce soit ».
