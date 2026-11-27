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
.\unreal\DeepSweep\Tools\run_python.ps1 -Script unreal\DeepSweep\Content\Python\ds_step0_map.py
```

Ordre de génération : `ds_materials.py` puis `ds_step0_map.py`.

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
- `ds.AutoPilot 1` : le plongeur nage seul en spirale.
- `ds.ShotAt 5` : capture d'écran à 5 s puis quitte (vérification automatique).

## Héberger / rejoindre
_À venir (étape 11 : menu Héberger/Rejoindre, OnlineSubsystemNull en LAN)._

## Avancement
- [x] Étape 0 — squelette C++ qui compile, carte vide générée en Python
- [x] Étape 1 — plongeur : nage arcade 6 axes, caméra 3e personne, Enhanced Input clavier + manette
