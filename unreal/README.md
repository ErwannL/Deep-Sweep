# Deep Sweep — projet Unreal Engine 5

Portage 3D du prototype (`../prototype/`) selon `../GDD.md`. Moteur : **UE 5.4**.

## Prérequis
- Unreal Engine 5.4 (`C:\Program Files\Epic Games\UE_5.4`)
- Visual Studio 2022 avec « Développement Desktop C++ » et « Développement de jeux C++ »
- Windows SDK 10.0.x

## Compiler
```powershell
.\unreal\DeepSweep\Tools\build.ps1
```
(ou ouvrir `DeepSweep.uproject` et accepter la recompilation)

## Générer / régénérer le contenu
Le contenu (cartes, matériaux, Data Assets…) est produit par des scripts Python éditeur dans `DeepSweep/Content/Python/` :
```powershell
.\unreal\DeepSweep\Tools\run_python.ps1 -Script unreal\DeepSweep\Content\Python\ds_materials.py
.\unreal\DeepSweep\Tools\run_python.ps1 -Script unreal\DeepSweep\Content\Python\ds_map_cove.py
```

Ordre : compiler d'abord (les scripts utilisent les classes C++), puis `ds_materials.py`, puis `ds_map_cove.py`.

> Au premier lancement, les shaders se compilent en arrière-plan : bulles et rayons peuvent n'apparaître qu'après quelques secondes.

## Lancer
Ouvrir `unreal/DeepSweep/DeepSweep.uproject` (carte de démarrage : `/Game/Maps/L_Cove`), puis Play.

## Contrôles
| Action | Clavier / souris | Manette |
|---|---|---|
| Nager | ZQSD ou WASD (ou flèches) | Stick gauche |
| Regarder | Souris | Stick droit |
| Monter / descendre | Espace / Ctrl gauche ou C | Gâchette droite / gauche |
| Attraper / interagir | F ou clic gauche | A |
| Action | G ou clic droit | X ou B |

## Commandes de debug (console `~`)
- `ds.AutoPilot 1` : le plongeur nage seul vers le centre de la crique.
- `ds.ShotAt 5` : capture d'écran à 5 s puis quitte (vérification automatique).
- `EnableCheats` puis `BugItGo x y z pitch yaw roll` : téléporte le plongeur.

## Héberger / rejoindre
_À venir (étape 11 : menu Héberger/Rejoindre, OnlineSubsystemNull en LAN)._

## Avancement
- [x] Étape 0 — squelette C++ qui compile, carte vide générée en Python
- [x] Étape 1 — plongeur : nage arcade 6 axes, caméra 3e personne, Enhanced Input clavier + manette
- [x] Étape 2 — crique 80×80 m : sable, dunes, rochers, coraux, algues, brouillard volumétrique bleu, rayons de lumière, bulles, zone de courant, colonne de bulles

![Crique](docs/step2_cove.png)
