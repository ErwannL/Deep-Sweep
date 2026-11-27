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
.\unreal\DeepSweep\Tools\run_python.ps1 -Script unreal\DeepSweep\Content\Python\ds_pickups.py
.\unreal\DeepSweep\Tools\run_python.ps1 -Script unreal\DeepSweep\Content\Python\ds_map_cove.py
```

Ordre : compiler d'abord (les scripts utilisent les classes C++), puis `ds_materials.py`, `ds_pickups.py` (Data Assets des objets), `ds_map_cove.py`.

## Architecture (GDD §14)
- **Objets** : `UDSPickupData` (Data Asset par type), `ADSPickupManager` (registre de la zone), `UDSPickupSubsystem` (accès + recherche de cible), `ADSPickup` (objet actif), `UDSBuoyancyComponent`.
- **Dormant** : rendu en instances (1 ISM par type), répliqué par *fast array* (seuls les changements partent sur le réseau).
- **Actif** : corps Chaos réveillé à < 6,5 m d'un plongeur (plafond 200), mouvement répliqué à 10 Hz ; se rendort après 3 s d'immobilité si aucun plongeur à < 9,5 m.
- **Monde** : `ADSWaterVolume` (courant / colonne de bulles), `UDSFlowParticlesComponent` (bulles et filets cosmétiques).

> Au premier lancement, les shaders se compilent en arrière-plan : bulles et rayons peuvent n'apparaître qu'après quelques secondes.

## Lancer
Ouvrir `unreal/DeepSweep/DeepSweep.uproject` (carte de démarrage : `/Game/Maps/L_Cove`), puis Play.

## Contrôles
| Action | Clavier / souris | Manette |
|---|---|---|
| Nager | ZQSD ou WASD (ou flèches) | Stick gauche |
| Regarder | Souris | Stick droit |
| Monter / descendre | Espace / Ctrl gauche ou C | Gâchette droite / gauche |
| Attraper / lâcher | F ou clic gauche | A |
| Lancer / action | G ou clic droit | X ou B |

## Commandes de debug (console `~`)
- `ds.Debug 1` : compteurs objets total / dormants / actifs / collectés.
- `ds.TestGiant 1` : scénario automatique porter le coffre jusqu'au comptoir.
- `ds.TestBelt 1` : scénario automatique cuve → tapis → blocage → déblocage → vente.
- `ds.TestVac 1` : scénario automatique prendre la buse → aspirer un tas.
- `ds.SetClean 0.5` : force la propreté affichée (−1 = valeur réelle).
- `ds.TestSell 1` : scénario automatique attraper → comptoir → vendre.
- `ds.TestGrab 1` : scénario automatique attraper → capture → lancer → capture → quitter.
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
- [x] Étape 3 — 150 objets de 6 types (Data Assets), flottabilité, système dormant/actif répliqué, code couleur, surbrillance au survol

![Objets](docs/step3_pickups.png)
- [x] Étape 4 — pince : attraper (objet visé), porter au-dessus de la tête (ralentit), lâcher, lancer vers la caméra ; validé par le serveur

![Pince](docs/step4_grab.png)
- [x] Étape 5 — comptoir (vente à la main ou en lançant dans la trémie), crédits partagés, stats par joueur, % de propreté qui fait passer la crique de sale à colorée, HUD

| Sale (0 %) | Propre (100 %) |
|---|---|
| ![Sale](docs/step5_dirty.png) | ![Propre](docs/step5_clean.png) |
- [x] Étape 6 — aspirateur à 2 postes : buse sur tuyau (câble physique), console de puissance 1–3 (5 avec turbo), cuve virtualisée, levier (Bob vide si personne), bouchon de caddie (3 tapes), vases qui cassent à puissance ≥ 4, collègues aspirés à puissance ≥ 4 (ressortent par la cuve, sonnés).

![Aspirateur](docs/step6_vacuum.png)
- [x] Étape 7 — convoyeur logique cuve → comptoir (spline + instances, éléments logiques répliqués par fast array, extrapolés côté client), blocage par caddie à mi-chemin + déblocage, objets lancés sur le tapis récupérés

![Convoyeur](docs/step7_conveyor.png)
- [x] Étape 8 — objets géants coop : coffre (2 porteurs, 300 ₡) et statue (3, 600 ₡) ; porteurs requis plafonnés au nombre de joueurs ; vitesse selon le nombre de porteurs ; direction = moyenne des intentions (rotation si désaccord) ; livraison au comptoir

![Coffre](docs/step8_giant.png)
