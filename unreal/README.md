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

## Lancer
Ouvrir `unreal/DeepSweep/DeepSweep.uproject` (carte de démarrage : `/Game/Maps/L_Cove`), puis Play.

## Héberger / rejoindre
_À venir (étape 11 : menu Héberger/Rejoindre, OnlineSubsystemNull en LAN)._

## Avancement
- [x] Étape 0 — squelette C++ qui compile, carte vide générée en Python
