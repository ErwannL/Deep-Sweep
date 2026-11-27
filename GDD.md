# DEEP SWEEP — Game Design Document

> *« Le fond de l'océan est un énorme tas. Videz-le. Ensemble. »*

Version 0.1 — Document de conception initial
Moteur cible : Unreal Engine 5 · Joueurs : 1 à 4 (optimal 2–4) · Genre : coop physique / incrémental / automatisation légère

---

## Sommaire

1. Présentation du jeu
2. Vision et identité
3. Analyse des jeux de référence
4. Boucle de gameplay
5. Mécaniques principales
6. Coopération
7. Physique aquatique
8. Outils et équipements
9. Machines et automatisation
10. Zones et progression
11. Économie
12. Événements aléatoires
13. Direction artistique
14. Architecture technique Unreal Engine
15. Prototype MVP
16. Roadmap de développement
17. Risques de conception et solutions

**Légende de faisabilité** utilisée partout : 🟢 simple · 🟡 moyen · 🔴 coûteux / risqué pour un petit studio.

---

## 1. Présentation du jeu

**Deep Sweep** est un jeu coopératif où 1 à 4 plongeurs, employés de la petite société *Abysso & Fils — Récupération en tout genre*, sont envoyés nettoyer des zones océaniques recouvertes de déchets, d'épaves et de trésors.

Au début, on ramasse des canettes avec une pince et un filet à papillon. À la fin, on pilote un sous-marin-usine qui avale des épaves entières pendant que des crabes robots trient des milliers d'objets sur des convoyeurs.

**Pitch en une phrase :** *Un « vide-le-tas » physique et coopératif sous l'eau, où le tas devient une usine, et l'usine devient un chaos à gérer à plusieurs.*

**Piliers :**
| Pilier | Traduction concrète |
|---|---|
| **Le tas qui fond** | Chaque zone affiche un % de propreté. Voir le fond réapparaître est la récompense principale. |
| **Les mains de tout le monde** | Les meilleurs gains exigent au moins 2 joueurs coordonnés (objets lourds, machines à postes). |
| **Physique drôle, jamais punitive** | L'eau rend tout flottant, glissant, rebondissant. Les accidents coûtent du temps, jamais la partie. |
| **Montée en puissance visible** | Chaque outil change *la façon* de jouer, et la base se transforme physiquement. |

**Format :** sessions de 20 à 60 min, sauvegarde persistante de la base, campagne de ~12–18 h, post-game de complétion à 100 %.

---

## 2. Vision et identité

### Fantasme du joueur
« Je suis un ouvrier maladroit mais ambitieux qui transforme une décharge sous-marine en empire industriel, avec mes potes qui me font tomber dans le broyeur. »

### Ton
- **Cartoon et bienveillant** : pas de mort. Un plongeur « K.O. » est une bulle qui remonte en râlant ; un collègue le récupère ou il réapparaît à la base après 10 s.
- **Satire légère** de l'entreprise : le patron (un vieux mérou en cravate, *M. Abysso*) envoie des mémos absurdes par radio (« Rappel : les requins ne sont pas des collègues »).
- **Écologique sans moraliser** : le monde redevient vivant à mesure qu'on nettoie (coraux qui reprennent leurs couleurs, poissons qui reviennent). C'est une récompense, pas une leçon.

### Ce qui différencie Deep Sweep
1. **La 3D verticale** : le tas n'est pas un tapis au sol, c'est un volume. Les objets flottent, coulent, remontent. On nettoie *en trois dimensions*.
2. **La flottabilité comme ressource** : on peut alléger un objet avec des bulles pour le transporter — c'est le cœur du transport coop.
3. **Une chaîne industrielle physique** : les objets circulent réellement sur les machines, peuvent tomber, bloquer, flotter hors des convoyeurs.
4. **Les machines à postes** : beaucoup de machines ont 2 à 4 sièges/rôles, comme dans une cuisine coopérative, mais avec des grues.

---

## 3. Analyse des jeux de référence

Les deux jeux cités (*Tas d'or* et *Vide un désert*, Aywen) appartiennent à la famille des jeux « **vider un énorme volume** » : un environnement saturé de matière que le joueur fait disparaître progressivement, avec des outils de plus en plus puissants achetés grâce à ce qu'il a collecté. Nous n'en reprenons aucun contenu ; nous en extrayons les principes.

| Principe observé | Pourquoi ça marche | Comment Deep Sweep l'amplifie |
|---|---|---|
| **Un objectif visible et mesurable** (le tas, le désert) | Le joueur voit immédiatement le travail restant et accompli. | Compteur de propreté par zone + transformation du décor (eau trouble → claire, fond gris → coraux colorés). |
| **Boucle courte collecte → vente → upgrade** | Récompense toutes les 30–90 s au début. | Même rythme, mais la vente devient une chaîne physique qu'on optimise. |
| **Saut de puissance des outils** | Passer de la pelle au gros engin est jouissif : le même travail prend 10× moins de temps. | Chaque palier change le verbe : *pincer → aspirer → grappiner → avaler*. |
| **Satisfaction sensorielle de masse** | Des centaines de particules/objets qui bougent ensemble. | Les objets sous l'eau tourbillonnent, s'agglutinent dans les tuyaux, tombent en cascade dans les compacteurs. |
| **Progression économique lisible** | Les prix montent mais les gains aussi ; on « sent » qu'on progresse. | Ajout de multiplicateurs de chaîne (combo de tri) et de trésors rares qui dynamisent l'économie. |
| **Simplicité d'accès** | On comprend en 10 secondes. | Une seule touche « attraper », une seule touche « utiliser l'outil ». La profondeur vient de la coop et des machines. |

**Limites des modèles que nous corrigeons :**
- Monotonie à long terme → zones avec mécaniques propres, événements aléatoires, objets géants.
- Expérience surtout solo → rôles coopératifs, machines à postes, objets nécessitant plusieurs joueurs.
- Peu d'interaction entre outils → les outils et machines s'emboîtent en chaînes.

---

## 4. Boucle de gameplay

### Trois boucles imbriquées

**Micro-boucle (10–60 s) — « Ramasser »**
Repérer → attraper/aspirer → remplir son sac/caisse → déposer. Feedback : *pop* sonore, compteur qui grimpe, objet qui s'envole vers l'inventaire.

**Boucle moyenne (5–15 min) — « Produire »**
Remplir la chaîne → trier → compacter → vendre → acheter une amélioration → retourner plus fort. Feedback : pluie de pièces au guichet de vente, jingle de déblocage.

**Macro-boucle (1–3 h) — « Transformer »**
Nettoyer une zone à 100 % → débloquer la suivante → revenir dans les anciennes avec les nouveaux outils (zones secrètes, objets lourds autrefois inaccessibles). Feedback : cinématique de « renaissance » de la zone, nouvelle pièce de base, trophée dans le bureau.

### Les 10 étapes demandées, intégrées
1. **Explorer** — sonar à impulsions (ping qui révèle les objets proches en surbrillance).
2. **Repérer** — code couleur universel : déchet (gris/bleu), recyclable (vert), métal (orange), précieux (doré, brillant), dangereux (rouge clignotant).
3. **Ramasser** — pince, filet, aspirateur, grappin…
4. **Transporter** — sac, caisse flottante, convoyeur, scooter.
5. **Trier** — à la main sur un établi, ou trieuse automatique.
6. **Vendre** — Guichet de vente (« Le Comptoir »), multiplicateur si les lots sont triés et compactés.
7. **Améliorer** — Atelier : capacité, puissance, autonomie d'oxygène.
8. **Construire** — mode construction (grille simple) pour poser convoyeurs et machines.
9. **Automatiser** — robots, drones, extracteurs.
10. **Débloquer** — permis de zone achetés + conditions de propreté.

### Courbe d'une session type (30 min)
- 0–5 min : on relance la chaîne, on répare ce qui a bouché.
- 5–20 min : expédition dans la zone, opération coop sur un objet géant.
- 20–25 min : un événement aléatoire perturbe tout.
- 25–30 min : vente, achats, petite cinématique de déblocage → envie de « juste une dernière plongée ».

---

## 5. Mécaniques principales

### 5.1 Le Tas (Pile System)
- **Fonctionnement :** chaque zone contient un volume de déchets : quelques centaines d'objets physiques « réels » proches des joueurs, et des milliers d'objets « dormants » (instanciés, non simulés) qui se réveillent à l'approche. Un % de propreté global s'affiche.
- **Fun :** la disparition progressive est la récompense centrale.
- **Coop :** le % est partagé ; tout le monde voit l'impact de chacun.
- **Évolution :** les tas deviennent plus gros, plus denses, plus verticaux (épaves empilées, colonnes de déchets flottants).
- **Faisabilité :** 🟡 — nécessite un système de LOD physique (voir §14).

### 5.2 L'inventaire physique
- **Fonctionnement :** pas d'inventaire abstrait illimité. Le plongeur a un sac (petite capacité) ; tout le reste se transporte physiquement (caisses, filets, convoyeurs).
- **Fun :** porter une caisse pleine qui déborde, voir des objets tomber, courir les rattraper.
- **Coop :** se passer des objets, faire une chaîne humaine.
- **Évolution :** sac → sac géant → sac lesté → téléporteur de sac (très tard).
- **Faisabilité :** 🟢.

### 5.3 La flottabilité contrôlée
- **Fonctionnement :** chaque objet a un poids apparent. Le plongeur peut lui coller des **ballons à bulles** (consommable bon marché) pour le rendre neutre ou flottant.
- **Fun :** une ancre de 2 tonnes qui se met à monter doucement comme un ballon de baudruche.
- **Coop :** un joueur gonfle, l'autre guide ; trop gonfler = l'objet file vers la surface, il faut le rattraper.
- **Évolution :** ballons → ballasts réutilisables → générateur de flottabilité de zone.
- **Faisabilité :** 🟢 — une force verticale ajustable.

### 5.4 L'oxygène comme rythme
- **Fonctionnement :** jauge d'O₂ qui fixe la durée d'une plongée. Recharge à la base, à des bulles naturelles, ou par un coéquipier (« bouche-à-bouteille »).
- **Fun :** tension douce, décisions « je pousse encore un peu ? ».
- **Coop :** partage d'oxygène, tuyau d'air relié à la base qu'un joueur doit dérouler.
- **Évolution :** bouteilles plus grandes, stations-relais, puis O₂ infini en sous-marin.
- **Faisabilité :** 🟢. *Option accessibilité : O₂ désactivable.*

### 5.5 Le combo de tri
- **Fonctionnement :** un lot homogène (100 % métal, par exemple) se vend avec un multiplicateur (×1,5 à ×3). Un lot mélangé se vend au prix « vrac ».
- **Fun :** optimiser, viser le lot parfait.
- **Coop :** spécialisation des rôles (le trieur devient un poste valorisé).
- **Évolution :** trieuses automatiques, puis contrats demandant des lots précis.
- **Faisabilité :** 🟢.

### 5.6 Les contrats
- **Fonctionnement :** tableau de missions : « Livrer 3 hélices intactes », « Nettoyer la crique nord sans casser de corail », « Récupérer le coffre du capitaine Brisebulle ».
- **Fun :** objectifs courts, variété.
- **Coop :** certains contrats sont uniquement coop (objets géants).
- **Évolution :** contrats de plus en plus industriels (quotas de production).
- **Faisabilité :** 🟢.

---

## 6. Coopération

### Principes de design coop
1. **Interdépendance, pas obligation** : en solo tout reste possible (via robots/outils d'assistance), mais à plusieurs c'est **plus rapide, plus rentable et plus drôle**.
2. **Rôles asymétriques temporaires** : on prend un poste sur une machine, puis on le quitte. Pas de classes figées.
3. **Communication encouragée** : signaux simples (ping, emotes, klaxon de casque), aucune obligation de micro.
4. **Les erreurs sont spectaculaires et récupérables.**

**Mise à l'échelle solo :** chaque machine à postes a un « mode assisté » (un robot Bob occupe un poste, moins efficace qu'un humain). Les objets géants solo nécessitent plus de ballons et plus de temps.

### 6.1 Mécaniques demandées

**GRUE-GRAPPIN GÉANT** 🟡
- *Fonctionnement :* 3 postes. **Grutier** (rotation et hauteur du bras, depuis la cabine), **Pinceur** (ouverture/fermeture et orientation des griffes, depuis une manette sur le bras), **Guide** (dehors, voit l'objet, pose des marqueurs que les autres voient en surbrillance). À 2 joueurs, guide optionnel.
- *Fun :* la caméra de cabine a une visibilité réduite (eau trouble) : le guide est indispensable. Le grappin balance avec l'inertie et le courant. Fermer trop tôt = l'objet est écrasé (perd de la valeur), trop tard = il glisse.
- *Coop :* communication constante, « gauche ! non TON gauche ! ».
- *Évolution :* grue fixe → grue sur rails → grue montée sur le sous-marin industriel.
- *Faisabilité :* physics constraint + câble (Cable Component). Grappin = 3 doigts à contraintes simples.

**ASPIRATEUR INDUSTRIEL** 🟡
- *Fonctionnement :* une cuve à la base, un tuyau segmenté de 30 m. Postes : **Porte-buse** (vise), **Porteur(s) de tuyau** (le tuyau pèse et accroche le décor, il faut le soulever aux virages), **Opérateur** (puissance 1–5 sur la cuve), **Videur** (cuve pleine = ouvrir la trappe vers le convoyeur).
- *Fun :* puissance 5 = la buse se cabre, tout est aspiré y compris les coéquipiers (ils ressortent dans la cuve, couverts de déchets, bonus « collègue recyclé » dans les stats de fin). Les gros objets bouchent le tuyau : il faut taper sur la bosse qui avance dans le tuyau.
- *Coop :* rythme opérateur/porte-buse ; vider à temps sinon débordement.
- *Évolution :* aspirateur dorsal (solo, petit) → industriel → requin aspirateur.
- *Faisabilité :* l'aspiration est une force radiale + « absorption » au contact (objet détruit, compté, ré-émis côté cuve). Le tuyau est une chaîne de 8–12 segments physiques. Pas de simulation de fluide.

**TRANSPORT D'OBJETS GÉANTS** 🟢
- *Fonctionnement :* statue, coffre, ancre, moteur, sous-marin abandonné… Chaque objet a des **poignées** (2 à 4). Tant que le nombre de porteurs est insuffisant, l'objet traîne au fond. On peut compenser avec des ballons. Les porteurs doivent nager dans la même direction : la vitesse est la moyenne des intentions ; si deux tirent en sens opposé, l'objet tourne sur lui-même.
- *Fun :* négociation de trajectoire, objets qui tapent le décor, coffre qui s'ouvre si secoué trop fort et répand des pièces.
- *Récompense :* valeur ×10 à ×50 d'un objet normal + souvent un déblocage (plan de machine, cosmétique).
- *Évolution :* objets de plus en plus gros → épaves complètes à découper puis remorquer.
- *Faisabilité :* attaches physiques multiples sur un corps rigide.

### 6.2 Dix (et plus) mécaniques coopératives originales

**1. La Chaîne Humaine de Bulles** 🟢
- *Fonctionnement :* en se tenant par la main (touche « agripper un coéquipier »), les plongeurs forment une chaîne. Le premier respire à une bulle d'air, l'oxygène circule dans toute la chaîne.
- *Fun :* exploration de grottes profondes en serpent humain, risque qu'un maillon lâche.
- *Coop :* indispensable dans la fosse abyssale.
- *Évolution :* remplacée plus tard par des tuyaux d'air, mais reste utile pour les zones secrètes.

**2. Le Treuil à Pédales** 🟢
- *Fonctionnement :* un treuil de remontée géré par 2 joueurs qui pédalent en rythme (alternance de touches). Synchro = vitesse ×2, désynchro = le câble se détend et l'objet retombe un peu.
- *Fun :* mini-jeu rythmique simple, hilarant à 4.
- *Évolution :* motorisé plus tard, mais le pédalage reste un « boost ».

**3. Le Scooter Tandem** 🟢
- *Fonctionnement :* scooter à deux places : pilote + passager qui tient un filet traînant ou un harpon. Le passager peut aussi « larguer » et se faire récupérer en vol.
- *Fun :* drive-by de déchets, cascades.
- *Évolution :* scooter → remorque à 4 places → sous-marin.

**4. La Cloche à Plongeur** 🟡
- *Fonctionnement :* une cloche d'air qu'on descend avec un câble. Un joueur en surface (sur le ponton) gère la descente et le ravitaillement, les autres l'utilisent comme base avancée (O₂, dépôt temporaire).
- *Fun :* le joueur de surface a une vraie mission (il voit la carte sonar, guide par radio, reçoit les caisses).
- *Évolution :* cloches multiples formant un réseau de bases avancées.

**5. Le Filet à Quatre Coins** 🟢
- *Fonctionnement :* un grand filet carré dont chaque coin est tenu par un joueur. On balaie un banc de déchets flottants, puis on referme en se rejoignant.
- *Fun :* moment de grâce chorégraphique quand ça marche, catastrophe quand quelqu'un lâche.
- *Coop :* 2 joueurs = filet rectangulaire plus petit ; 4 = giga-filet.
- *Évolution :* giga-filet motorisé, puis drones porte-filet.

**6. Le Découpage d'Épave** 🟡
- *Fonctionnement :* les épaves sont découpées le long de **lignes de découpe** pointillées. Un joueur découpe au chalumeau pendant qu'un autre maintient la pièce avec un aimant, sinon la plaque tombe sur le découpeur (K.O. comique) ou se perd dans un ravin.
- *Fun :* démontage satisfaisant, prévisible (lignes visibles).
- *Évolution :* découpeur laser, puis broyeur d'épaves.
- *Faisabilité :* épaves pré-fracturées en morceaux (pas de découpe procédurale).

**7. L'Éclaireur et le Phare** 🟢
- *Fonctionnement :* dans les zones sombres, un joueur porte le **projecteur puissant** (lent, encombrant), les autres travaillent dans son cône de lumière. Hors de la lumière, les objets sont invisibles et des créatures s'approchent.
- *Fun :* rôle de « maître de la lumière », tension douce.
- *Évolution :* projecteurs fixes, drones-lampes, puis la cité engloutie qui s'illumine.

**8. Le Bouchon Humain** 🟢
- *Fonctionnement :* quand une conduite ou une machine fuit (bulles, courant inverse), un joueur peut littéralement s'y coller pour la boucher pendant qu'un autre répare.
- *Fun :* absurde, crée des moments d'entraide urgents.
- *Évolution :* kits de réparation, robots réparateurs.

**9. Le Lancer de Plongeur (Catapulte à Bulles)** 🟢
- *Fonctionnement :* un joueur s'installe dans la catapulte, un autre règle l'angle et tire. Le projeté traverse les courants qui bloqueraient une nage normale.
- *Fun :* transport express, ratés spectaculaires.
- *Coop :* permet d'atteindre des zones d'accès et d'accrocher un câble pour les autres.

**10. L'Attelage de Ballast** 🟡
- *Fonctionnement :* pour soulever un objet colossal, il faut répartir des ballons sur plusieurs points. Si la répartition est déséquilibrée, l'objet bascule et se retourne. Un joueur surveille le « niveau à bulle » (indicateur d'équilibre), les autres ajustent.
- *Fun :* puzzle physique collectif.

**11. Le Relais de Convoyeur Manuel** 🟢
- *Fonctionnement :* avant d'avoir des convoyeurs, des « tubes pneumatiques » portatifs doivent être tenus et alignés par les joueurs pour former une ligne. Si un joueur bouge, le flux sort dans l'eau.
- *Fun :* chaîne humaine improvisée, prélude à l'automatisation.

**12. Le Dompteur de Créature** 🟡
- *Fonctionnement :* certaines créatures (raie manta, tortue géante) peuvent être « attelées ». Un joueur la calme en la nourrissant (algues récoltées par un autre), un autre la guide.
- *Fun :* transport vivant, imprévisible.

**13. Le Sous-Marin à Postes** 🔴 (fin de jeu)
- *Fonctionnement :* 4 postes : **pilote**, **bras mécanique**, **ingénieur** (répartit l'énergie entre moteurs/aspiration/lumières/bouclier, éteint les incendies), **soute** (trie ce qui est avalé). Inspiré d'une logique de « vaisseau coopératif ».
- *Fun :* climax de la coop, chaque rôle est crucial.

**14. La Pesée du Comptoir** 🟢
- *Fonctionnement :* le Comptoir de vente a une balance : un lot pile au poids d'un contrat donne un bonus. Les joueurs ajoutent/retirent objets en temps réel avant de valider.
- *Fun :* petite négociation collective.

### 6.3 Fun garanti de 1 à 4 joueurs

Règle de conception : **aucun contenu n'est bloqué par le nombre de joueurs, mais chaque joueur en plus ouvre de nouvelles stratégies.**

| Système | Solo | 2 joueurs | 3–4 joueurs |
|---|---|---|---|
| **Postes de machine** | Bob le robot tient les postes vides (puissance par défaut, vidage auto lent). Le joueur alterne les postes. | Un joueur à la buse, l'autre gère console/levier/convoyeur. | Spécialisation complète, Bob devient inutile. |
| **Objets géants** | Portables seul mais lents (~35–60 % de vitesse), ballons recommandés. | Coffres à pleine vitesse, statues à ~80 %. | Tout à pleine vitesse, objets colossaux réservés. |
| **Densité de la zone** | Même zone, mêmes objets : le défi est le temps, pas la difficulté. | — | Événements plus fréquents pour garder le chaos. |
| **Économie** | Prix des upgrades personnels identiques. | Upgrades personnels −15 %. | −25 % (évite de payer 4 fois). |
| **Accidents** | Contre soi-même (s'aspirer est impossible, mais casser des vases oui). | Aspirer son collègue, se bloquer. | Chaos maximal, stats de fin humoristiques. |
| **Caméra (local)** | Suit le joueur. | Caméra partagée qui dézoome. | Idem, écran partagé en option en ligne. |

Le prototype applique déjà ces règles (Bob, porteurs plafonnés, caméra partagée).

### 6.4 Outils sociaux
- **Ping contextuel** (objet, danger, « viens ici », « j'ai besoin de bras »).
- **Klaxon de casque** et emotes (pouce, panique, danse de victoire).
- **Tableau de fin de session** humoristique : « Plus grand nombre de collègues aspirés », « A cassé le plus de vases antiques ».

---

## 7. Physique aquatique

Règle d'or : **physique stylisée, contrôle précis du joueur**. Le personnage se déplace de manière réactive (nage arcade 6 axes), seul l'environnement et les objets ont une physique « molle ».

| Mécanique | Fonctionnement | Moments mémorables | Faisabilité |
|---|---|---|---|
| **Flottabilité** | Chaque objet : valeur de flottabilité (-1 coule, 0 neutre, +1 remonte). Modifiable par ballons/ballasts. | La statue qui s'envole quand on met trop de ballons. | 🟢 |
| **Courants marins** | Volumes de force directionnelle, visibles (particules, algues penchées). | Le courant emporte une caisse entière vers un ravin, tout le monde plonge pour la rattraper. Les courants peuvent aussi servir de **convoyeurs naturels**. | 🟢 |
| **Pression** | Par palier de profondeur, l'équipement doit être upgradé. Sans upgrade : vision qui se rétrécit, ralentissement (pas de mort). Les objets fragiles remontés trop vite explosent en confettis. | « Décompression » d'un vase antique remonté trop vite : il éclate en bulles. | 🟢 |
| **Bulles d'air** | Sources naturelles : recharge O₂ et propulsion verticale (on peut « surfer » une colonne de bulles). | Ascenseurs à bulles naturels pour le transport. | 🟢 |
| **Objets qui coulent/remontent** | Les déchets plastiques flottent vers le plafond des grottes, les métaux coulent. | Nettoyer « le plafond » d'une grotte, objets coincés sous les rochers. | 🟢 |
| **Algues** | Les algues s'enroulent autour des pièces mobiles (hélices, buses, convoyeurs) et réduisent leur efficacité. On les coupe (ressource : algues → vendues ou nourriture de créatures). | Le convoyeur ralentit, puis s'arrête complètement, emmêlé. | 🟡 |
| **Visibilité** | Brouillard volumétrique par zone, sédiments soulevés par les machines lourdes. | L'aspirateur à puissance max soulève un nuage, plus personne ne voit rien pendant 10 s. | 🟢 |
| **Tourbillons** | Volume de force rotationnelle + aspiration centrale. | Un tourbillon centralise les déchets (utile !) mais aussi les joueurs. | 🟢 |
| **Créatures** | IA simple (comportements : curieux, voleur, territorial, peureux). | Le poulpe voleur, le banc de poissons qui pousse les objets. | 🟡 |
| **Explosions de bulles** | Les bulles de méthane (volcan, abysses) éclatent au contact d'outils chauds : onde de choc radiale qui disperse tout. Peut être utilisé volontairement pour « dégager » un tas compact. | Chaîne d'explosions en série qui projette des centaines d'objets. | 🟢 |

**Règles anti-frustration :**
- Le joueur n'est jamais poussé plus de quelques mètres par un courant sans pouvoir réagir (vitesse de nage > courant moyen).
- Un objet perdu « hors limite » réapparaît sur un **tapis de récupération** à la base avec une petite pénalité.
- Pas de dégâts de chute, pas de noyade : à 0 O₂, le plongeur est « K.O. bulle ».

---

## 8. Outils et équipements

Chaque outil introduit un **nouveau verbe**, puis s'améliore en 3 niveaux qui changent le comportement (pas seulement les stats).

### Début du jeu
| Outil | Verbe | Ce qu'il change | Niveaux |
|---|---|---|---|
| **Pince** | Attraper précis | Ramasse 1 objet, peut lancer. | 1 : simple · 2 : pince télescopique (portée ×2) · 3 : pince double (2 objets, ou un objet lourd seul) |
| **Filet** | Balayer | Ramasse les petits objets flottants en zone. | 1 : filet main · 2 : filet traînant (on nage, il récolte) · 3 : filet auto-refermant qui se lie en ballot |
| **Petite caisse** | Stocker/transporter | 20 slots, flotte légèrement. | 1 : caisse · 2 : caisse à hélice (suit le joueur) · 3 : caisse qui se vide seule sur un convoyeur |
| **Lampe torche** | Révéler | Cône de lumière, révèle objets cachés. | 1 : lampe · 2 : lampe UV (révèle trésors) · 3 : lampe frontale + sonar |

### Milieu du jeu
| Outil | Verbe | Nouveau gameplay |
|---|---|---|
| **Aspirateur dorsal** | Aspirer en masse | Petit objets en flux continu ; risque de bouchon. |
| **Grappin motorisé** | Tirer à distance | Ramène un objet moyen, ou tire le joueur vers un point (mobilité). |
| **Scooter sous-marin** | Se déplacer vite | Transport rapide, peut remorquer une caisse. Tandem possible. |
| **Sac géant** | Stocker énormément | 200 slots mais ralentit beaucoup → on le confie à un « porteur ». |
| **Harpon magnétique** | Aimanter | Tire un aimant qui attire tous les métaux dans un rayon. Les objets s'agglutinent en boule de métal → à rapporter. |

### Fin du jeu
| Outil | Verbe | Nouveau gameplay |
|---|---|---|
| **Grue industrielle** | Soulever l'énorme | Opération coop à postes (§6). |
| **Drones de récupération** | Déléguer | Programmation simple : zone + type d'objet + dépôt. |
| **Extracteurs géants** | Exploiter un gisement | Machines posées sur un tas, qui l'aspirent seules vers un convoyeur. |
| **Sous-marin industriel** | Avaler | Base mobile à 4 postes. |
| **Usine flottante** | Tout centraliser | Le hub qui remplace la base, transportable entre zones. |

### Équipements absurdes (débloqués par contrats/trésors)
- **Ventilateur des abysses** 🟢 — une hélice géante posée au sol qui crée un courant artificiel. *Usage :* rassembler tous les déchets d'une zone dans un coin, ou… envoyer les coéquipiers dans l'autre sens.
- **Canon à bulles** 🟢 — tire une bulle qui enferme un objet et le fait monter, puis éclate au contact. *Usage :* expédier des déchets « par la voie des airs » vers la base (arc balistique inversé, puisqu'ils remontent).
- **Giga-filet** 🟡 — filet tendu entre deux scooters, ratisse des centaines d'objets. Coop obligatoire.
- **Requin aspirateur (« Glouton »)** 🟡 — un requin robot qu'on chevauche : il avale tout devant lui, mais il faut le « vider » régulièrement (il régurgite tout d'un coup, parfois au mauvais endroit). Il a faim : s'il n'avale rien pendant 20 s, il part chercher seul (y compris dans la cuve de vente…).
- **Crabe industriel automatisé (« Pinçou »)** 🟡 — robot crabe qui marche latéralement le long d'un parcours, ramasse et trie selon sa couleur de pince. Peut être « customisé » (pinces de tri, pinces de découpe).
- **Bouteille de soda pressurisée** 🟢 — jetpack instable, ultra rapide, trajectoire aléatoire.
- **Pistolet à colle-corail** 🟢 — colle des objets ensemble en « boule de déchets » (pour transporter beaucoup d'un coup, ou bloquer un convoyeur en cas d'erreur).
- **Poisson-lanterne apprivoisé** 🟢 — compagnon qui suit un joueur et éclaire. Il a peur de l'aspirateur.

---

## 9. Machines et automatisation

### Principes
- **Objets réels** : les objets sur les machines sont des objets physiques (jusqu'à un seuil, au-delà « bulking », voir §14).
- **Pose sur grille souple** : grille de 1 m, rotation 90°, snapping entre ports d'entrée/sortie.
- **Chaque machine peut échouer** de façon lisible (voyant, son, fumée de bulles) et se réparer en un geste.

### Catalogue
| Machine | Rôle | Panne typique | Interaction joueur |
|---|---|---|---|
| **Convoyeur** | Transport linéaire, courbes, rampes | Objet qui flotte et sort, algues | Débloquer à la main, remettre les objets |
| **Tube pneumatique** | Transport rapide vertical/longue distance | Bouchon par objet trop gros | Taper sur la bosse |
| **Trémie d'entrée** | Point de dépôt universel | Débordement | Vider, ou ajouter une 2e trémie |
| **Station de tri manuel** | Établi à 4 bacs | Rien — c'est un poste joueur | Mini-jeu rapide de tri (attraper et jeter dans le bon bac) |
| **Trieuse automatique** | Tri par catégorie (scanner coloré) | Erreurs si surchargée → objets mal triés | Régler la vitesse, nettoyer le scanner |
| **Broyeur** | Réduit les objets en « granulés » (plus faciles à transporter, valeur ×1,2) | Objet indestructible bloque les lames | Retirer avec la pince (risque de se faire aspirer) |
| **Compacteur** | Transforme 20 objets du même type en un cube (×1,5 valeur, transport facile) | Mélange = cube « bâtard » sans bonus | Bien alimenter |
| **Fondeuse** (volcan) | Métal → lingots (×2) | Surchauffe | Refroidir avec de l'eau froide (pompe) |
| **Réservoir** | Tampon de stockage | Plein = toute la chaîne amont s'arrête | Ajouter de la capacité |
| **Comptoir de vente** | Convertit en crédits | Jamais en panne, mais a une « saturation » de marché | Choisir quoi vendre |
| **Expéditeur à bulles** | Envoie les cubes à la surface (vente à distance) | Bulle qui éclate à mi-chemin | Escorter / upgrade |
| **Robot Bob** | Porte des objets entre 2 points | Se coince, se perd | Le remettre sur ses rails, lui donner une tape |
| **Grue fixe** | Charge les gros objets sur la chaîne | Mal opérée | Poste coop |
| **Extracteur** | Aspire un tas automatiquement | Algues, gros objets | Nettoyage périodique |
| **Désalgueur** | Coupe les algues sur les machines voisines | — | Contre-mesure de la forêt d'algues |

### Exemple de chaîne (milieu de jeu)
```
[Tas] → Aspirateur industriel → Cuve → Convoyeur → Trieuse
                                                     ├→ Métal → Compacteur → Comptoir
                                                     ├→ Plastique → Broyeur → Comptoir
                                                     └→ Précieux → Coffre-fort (vente manuelle ×3)
```
Avec 4 joueurs : **l'aspireur**, **le videur/réparateur**, **le trieur** (tri manuel des précieux pour bonus), **le transporteur/vendeur**.

### Automatisation progressive (paliers)
1. **Tout à la main** (heure 0–1).
2. **Convoyeur + trémie** : on n'a plus à marcher jusqu'au Comptoir (heure 1).
3. **Trieuse + compacteur** : la base produit pendant qu'on explore (heure 3).
4. **Robots/drones** : la collecte elle-même devient partiellement automatique (heure 6).
5. **Extracteurs + usine flottante** : les joueurs deviennent des managers-pompiers qui gèrent les crises et font les opérations coop de prestige (heure 10+).

> **Garde-fou** : l'automatisation ne remplace jamais les **objets géants** et les **trésors**, qui exigent des joueurs. L'usine nettoie le menu fretin, les humains s'occupent de l'épique.

---

## 10. Zones et progression

Chaque zone : un **gimmick physique**, un **outil signature**, un **objet géant emblématique**, un **boss-événement** de fin, et des **secrets** accessibles plus tard.

### 1. Le Lagon — *« Premier jour »*
- **Ambiance :** eau turquoise, sable blanc, 0–15 m.
- **Mécanique :** apprentissage, flottabilité de base, courants doux.
- **Déchets :** canettes, bouteilles, tongs, pneus, caddies.
- **Objet géant :** un pédalo-cygne échoué (2 joueurs).
- **Fin de zone :** le « Grand Tas du Lagon » (un monticule de 1 000 objets) à faire disparaître.
- **Secret :** grotte sous le récif accessible avec la lampe UV.
- **Durée :** 1–1,5 h.

### 2. Le Cimetière des Bateaux — *« Le poids du métal »*
- **Ambiance :** brume verte, épaves rouillées, 15–40 m.
- **Mécanique :** objets lourds, **découpage d'épave**, aimants, ballons.
- **Outils signature :** harpon magnétique, grue fixe.
- **Objet géant :** une ancre de paquebot, un moteur de bateau, une cloche de navire.
- **Fin de zone :** démanteler le *Rouille-Majesté*, un cargo entier (opération à étapes).
- **Durée :** 2 h.

### 3. La Forêt d'Algues — *« Tout s'emmêle »*
- **Ambiance :** colonnes de kelp géantes, lumière dorée filtrée.
- **Mécanique :** les algues **repoussent** et emmêlent machines et joueurs ; visibilité par couloirs ; les algues sont aussi une ressource (nourriture de créatures, matériau de cordes).
- **Outils signature :** cisaille rotative, désalgueur, filet à quatre coins.
- **Créature :** les loutres de mer chapardeuses, les hippocampes à atteler.
- **Fin de zone :** libérer la « Baleine prisonnière » emmêlée dans des filets fantômes (opération coop, récompense : la baleine devient un « ascenseur » vers la surface).
- **Durée :** 2 h.

### 4. Le Volcan Sous-Marin — *« Ça chauffe »*
- **Ambiance :** roches noires, lueurs orangées, cheminées.
- **Mécanique :** **zones de chaleur** (surchauffe des outils), **courants ascendants violents**, **bulles explosives**, minerais rares qui ne peuvent être récupérés qu'avec la fondeuse.
- **Outils signature :** fondeuse, pompe à eau froide, combinaison thermique.
- **Objet géant :** un œuf de géode (s'ouvre si chauffé = cristaux).
- **Fin de zone :** canaliser une coulée de lave via des courants pour former une plateforme de base.
- **Durée :** 2–2,5 h.

### 5. La Fosse Abyssale — *« Le noir »*
- **Ambiance :** noir presque total, bioluminescence, pression extrême.
- **Mécanique :** **visibilité quasi nulle** (éclaireur indispensable), **pression** qui exige des paliers, chaîne humaine, créatures attirées par la lumière.
- **Outils signature :** projecteur, cloche à plongeur, sonar avancé.
- **Objet géant :** le sous-marin abandonné *Nautilette* (devient le sous-marin industriel une fois réparé !).
- **Fin de zone :** remonter la *Nautilette* en entier.
- **Durée :** 2,5 h.

### 6. La Cité Engloutie — *« L'usine des Anciens »*
- **Ambiance :** ruines monumentales, mosaïques, machines antiques luisantes.
- **Mécanique :** trésors anciens fragiles, **machines anciennes** à réactiver (convoyeurs de pierre, ascenseurs à eau), énigmes coop (leviers simultanés).
- **Objet géant :** statues colossales, coffre du roi englouti.
- **Fin de jeu :** réactiver le « Cœur de la Cité » : une usine antique gigantesque qui se connecte à votre chaîne et la propulse au niveau ultime.
- **Durée :** 3 h.

### Raisons de revenir dans les anciennes zones
- **Objets « verrouillés »** visibles dès le début (une ancre trop lourde dans le Lagon, une épave scellée) — ils deviennent accessibles avec les nouveaux outils.
- **Zones secrètes** (grottes UV, passages à haute pression).
- **Contrats rejouables** et **« marées »** : chaque semaine en jeu, la mer redépose des déchets dans les zones propres (léger, optionnel, idéal pour les robots).
- **Collection** : musée de la base qui expose les trésors trouvés (100 % complétion).
- **Écosystème** : chaque zone nettoyée à 100 % fait revenir des espèces, qui débloquent des bonus (dauphins livreurs, tortues porteuses).

---

## 11. Économie

### Monnaies et ressources
| Ressource | Source | Usage |
|---|---|---|
| **Crédits (₡)** | Vente de tout | Achats d'outils, machines, upgrades |
| **Matériaux** (ferraille, plastique, verre, algues, minerai) | Tri (on peut les **garder** au lieu de les vendre) | Coût secondaire de construction des machines |
| **Plans** | Objets géants, trésors, contrats | Débloquent machines/outils (on ne peut pas tout acheter juste avec des ₡) |
| **Réputation** | Contrats, % de propreté | Débloque zones et meilleurs contrats |
| **Perles** (rare) | Trésors, secrets | Cosmétiques et outils absurdes |

Le choix **vendre vs garder un matériau** crée une petite décision économique permanente.

### Ordres de grandeur (à équilibrer en playtest)
| Objet | Valeur base |
|---|---|
| Canette | 1 ₡ |
| Bouteille | 2 ₡ |
| Pneu | 8 ₡ |
| Pièce métallique d'épave | 25 ₡ |
| Lingot (fondu) | 120 ₡ |
| Trésor moyen (vase, bijou) | 300–800 ₡ |
| Objet géant | 2 000–50 000 ₡ |

**Multiplicateurs :** tri homogène ×1,5 · compacté ×1,5 · fondu ×2 · contrat ×1,2–2 · combo de chaîne (débit continu) jusqu'à ×1,3.

### Courbe de coûts
- Coût d'un upgrade de rang *n* : `C(n) = base × 1,6^n` (croissance exponentielle douce).
- Revenus horaires : croissance d'environ ×3 à chaque zone grâce aux nouveaux outils et multiplicateurs.
- **Objectif ressenti :** premier achat en < 2 min ; un upgrade majeur toutes les 10–15 min ; un « saut de puissance » (nouvel outil signature) toutes les 45–60 min.

### Partage en coop
- **Porte-monnaie commun** pour la base et les machines (évite les disputes).
- **Prime personnelle** (10 % de ce que chacun rapporte) pour les cosmétiques et outils personnels.
- Les **upgrades personnels** (O₂, vitesse) sont achetés pour chaque joueur, mais à coût réduit par nombre de joueurs pour éviter la sensation de payer 4 fois.

### Agrandissement de base
La base est un ponton + dôme sous-marin, extensible par **modules** (atelier, musée, dortoir des robots, quai à sous-marin, labo de plans). Chaque module est visible et transforme physiquement le hub.

### Anti-inflation / anti-farm
- **Saturation de marché** légère : vendre 500 canettes d'affilée fait baisser temporairement leur prix (−20 % max), ce qui pousse à diversifier.
- Les marées limitent le farming de zones vides.

---

## 12. Événements aléatoires

Règles : **annoncés** (radio de M. Abysso, signe visuel 5–10 s avant), **limités dans le temps** (30 s à 2 min), **résolubles** (et souvent rémunérateurs si résolus), **désactivables/réglables** dans les options.

### Événements de base
1. **Le Poulpe voleur** — vole les objets précieux et les cache dans sa grotte. Le poursuivre → il lâche un nuage d'encre ; on récupère tout son butin (bonus).
2. **Le Tourbillon** — aspire les déchets en un point. Pénible pour la chaîne, mais pratique : on peut s'en servir pour rassembler.
3. **Le Crabe géant** — s'installe sur une machine (il aime la chaleur). Il faut l'attirer avec de la nourriture ou le soulever à 2.
4. **Le Coffre piégé** — explose en milliers de pièces qui se dispersent. Course collective au ramassage.
5. **La Tempête sous-marine** — houle qui déplace tous les objets non fixés.
6. **L'Invasion de poissons** — un banc traverse les convoyeurs et emporte des objets.

### 15+ événements supplémentaires
7. **La Marée Noire de Paillettes** — un conteneur de paillettes éclate : visibilité réduite, mais tout ce qui brille est révélé.
8. **Le Banc de Méduses** — décharges légères qui font lâcher les objets portés. Il faut contourner ou les aspirer à puissance faible.
9. **La Livraison surprise** — un cargo en surface lâche une palette de conteneurs : objets précieux, à récupérer avant qu'ils ne glissent dans un ravin.
10. **La Grève des Robots** — les Bob s'arrêtent et réclament… une bonne tape chacun (ou de l'huile). Mini-défi de réactivation.
11. **Le Séisme** — des rochers s'effondrent et révèlent un passage secret, mais secouent toute la chaîne.
12. **La Baleine curieuse** — elle avale le contenu d'un réservoir et repart. La suivre : elle le recrache plus loin… avec un bonus.
13. **Le Concours de M. Abysso** — 2 minutes : la catégorie X vaut ×3. Tout le monde se rue.
14. **La Panne de courant** — la base s'éteint, il faut pédaler au générateur pendant que d'autres relancent les machines.
15. **Le Bouchon Géant** — un objet géant tombe dans l'aspirateur et le bouche : opération d'extraction à 2.
16. **La Migration des tortues** — elles traversent la zone : interdit de les gêner (bonus « écolo » si aucun contact), mais elles peuvent porter des objets si on les attelle.
17. **La Bulle de méthane géante** — elle grossit ; si on ne la perce pas au bon endroit, l'explosion disperse le tas trié.
18. **L'Inspecteur du Port** — un poisson-ballon en képi inspecte la base : si tout est rangé (aucun objet perdu au sol), grosse prime.
19. **L'Épave fantôme** — une épave lumineuse apparaît 3 min avec des trésors rares, puis disparaît.
20. **Le Requin collectionneur** — il vole les cubes compactés pour sa « collection ». On peut négocier en lui laissant les cubes « bâtards ».
21. **Le Courant de Retour** — les courants s'inversent pendant 1 min : les convoyeurs « naturels » fonctionnent à l'envers.
22. **La Pluie de Pièces** — un galion en surface sombre : des pièces tombent du « ciel » liquide partout.

**Système de tirage :** un « directeur d'événements » choisit selon la zone, le nombre de joueurs, le temps depuis le dernier événement (min 6–8 min) et l'état de la chaîne (pas d'événement destructeur si la chaîne est déjà en crise).

---

## 13. Direction artistique

### Style
- **3D stylisée low/mid-poly**, shading cel-léger avec contours doux, couleurs saturées. Références d'esprit : jouets en plastique, dessins animés du samedi matin, maquettes de chantier.
- **Lisibilité avant tout** : silhouettes simples, objets surdimensionnés (une canette fait 2× sa taille réelle), code couleur systématique par catégorie + contour lumineux au survol.

### Personnages
- Plongeurs trapus, gros casques ronds à hublot qui montrent des yeux très expressifs (animation d'yeux et sourcils = 80 % de l'émotion).
- Combinaisons personnalisables (couleurs, palmes, casques absurdes : casque-bocal avec poisson dedans, casque-scaphandre rétro).
- Animations exagérées : moulinet de bras quand on est aspiré, gonflement quand on prend trop de bulles.

### Machines
- Inspirées de l'industrie rétro-futuriste « bricolée » : rivets, tuyaux colorés, cadrans à aiguille, ampoules qui clignotent, pièces de récup visibles (une machine est faite avec une baignoire).
- Les machines **réagissent** : elles tremblent quand elles forcent, crachent des bulles, rebondissent quand elles avalent un gros objet (*squash & stretch*).

### Environnements
- Palettes par zone : Lagon (turquoise/sable), Cimetière (vert-gris/rouille), Forêt (or/émeraude), Volcan (noir/orange), Abysses (indigo/cyan bioluminescent), Cité (bleu nuit/or/marbre).
- **Avant/après** : chaque zone a deux états de matériaux (sale : désaturé, trouble ; propre : couleurs vives, rayons de soleil), interpolés par le % de propreté.

### Feedback sensoriel (juice)
- Chaque objet ramassé : *pop* + petit son au pitch croissant en combo.
- Compteur de crédits qui « roule » comme une machine à sous.
- Caméra qui tremble légèrement à l'arrivée d'un objet géant.
- Musique dynamique : couche percussive industrielle qui s'ajoute quand la chaîne tourne à plein régime.

### Ambiance « mystérieux et chaotique »
- Mystère : silhouettes lointaines, chants de baleine, ruines au loin dès le Lagon (on voit la Cité au fond d'un ravin, inaccessible).
- Chaos : alarmes clignotantes, objets qui volent, radio du patron qui panique.

---

## 14. Architecture technique Unreal Engine

### Choix généraux
- **UE 5.x**, C++ pour les systèmes cœur (physique d'objets, réseau, chaîne), **Blueprints** pour le contenu (objets, machines, événements, UI).
- **Physique :** Chaos Physics.
- **Réseau :** modèle **listen server** (un joueur héberge), via **Steam Sessions** / Online Subsystem Steam (ou EOS pour le crossplay plus tard). Pas de serveurs dédiés → coût nul pour le studio.
- **Rendu :** Lumen optionnel (désactivable), brouillard volumétrique, Niagara pour bulles/particules.

### Le problème central : synchroniser des centaines d'objets physiques
Répliquer 1 000 corps rigides en réseau est impossible proprement. Solution en **3 niveaux de simulation** :

| Niveau | Description | Réseau |
|---|---|---|
| **Dormant** | Objet instancié (Instanced Static Mesh / HISM), aucune physique, position fixe stockée dans un tableau. | Seul un **identifiant + état (présent/collecté)** est répliqué, via un bitset compressé. Coût quasi nul. |
| **Actif** | Corps rigide simulé, seulement près des joueurs/machines (rayon ~15 m), **plafond global** (~150–250 actifs). | Simulé **sur le serveur** ; réplication de mouvement **à basse fréquence** (5–10 Hz) avec interpolation côté client. Les objets peu importants peuvent être simulés localement côté client (« cosmétiques ») avec correction au repos. |
| **Porté / critique** | Objet tenu par un joueur, objet géant, objet sur machine. | Réplication prioritaire (Replication Graph / priorités), haute fréquence. |

- **Retour au repos :** un objet actif immobile depuis 3 s redevient dormant (sa nouvelle position est écrite dans l'ISM).
- **Flux de masse (aspirateur, convoyeur)** : au-delà d'un seuil, les objets sont **« virtualisés »** : l'aspirateur détruit l'objet et incrémente un compteur ; côté visuel, un système Niagara montre un flux d'objets « fantômes » non physiques. Les clients voient des centaines d'objets voler sans aucun coût réseau.
- **Convoyeurs :** les objets en transit sont des **éléments logiques** (position sur une spline + type), rendus en ISM. Ils ne redeviennent physiques que s'ils **tombent** (événement explicite, ex : convoyeur coupé). Exactement le modèle des jeux d'automatisation : déterministe, léger, synchronisable.

### Modules principaux (C++)
- `UDSPickupSubsystem` — registre de tous les objets de la zone (dormants/actifs), activation/désactivation, sauvegarde.
- `ADSPickup` — objet actif (données : type, valeur, poids, flottabilité, fragilité).
- `UDSBuoyancyComponent` — force verticale + amortissement, ballons attachés.
- `ADSWaterVolume` — courants, tourbillons, visibilité, pression (volumes simples à forces).
- `UDSCarryComponent` — gestion des poignées multi-joueurs, calcul de force combinée.
- `ADSMachine` (base) + ports d'entrée/sortie, état (OK/bloqué/plein/en panne), **sièges** répliqués (`UDSMachineSeatComponent`).
- `UDSConveyorNetwork` — graphe logique des convoyeurs, simulation tick fixe côté serveur, réplication par segments.
- `UDSEconomySubsystem` (serveur) — crédits, multiplicateurs, saturation.
- `UDSEventDirector` — tirage et orchestration des événements.
- Sauvegarde : `USaveGame` côté hôte (base, machines, % propreté, bitset des objets collectés).

### Contrôle du personnage
- `CharacterMovementComponent` en mode **Flying/Swimming** personnalisé, prédiction côté client native → déplacements fluides même avec latence.
- Les interactions physiques fortes (être aspiré, courants) passent par des **forces appliquées côté serveur avec prédiction simple** (root motion sources ou impulsions corrigées).

### Données et contenu
- **Data Assets** pour chaque type d'objet, outil et machine (designer-friendly, équilibrage via Data Tables/CSV).
- **Gameplay Tags** pour catégories (Métal, Plastique, Précieux, Fragile, Géant).
- **Enhanced Input** + support manette complet.
- **World Partition** pour les grandes zones ; chaque zone = un niveau distinct streamé depuis le hub au début (plus simple qu'un open world).

### Budget de performance cible
- 60 fps sur GPU milieu de gamme (GTX 1660 / RTX 2060), 4 joueurs.
- ≤ 250 corps physiques actifs, ≤ 20 000 objets dormants par zone, ≤ 50 machines actives.
- Bande passante ≤ 64 Ko/s par client.

### Outils de debug essentiels (dès le prototype)
- Visualisation des niveaux de simulation (couleurs), compteur d'objets actifs, simulation de latence/pertes (`Net PktLag`, `PktLoss`), mode « 4 joueurs » en PIE.

---

## 15. Prototype MVP

**Objectif :** répondre à **une seule question** : *« Est-ce que vider une zone à plusieurs, avec un aspirateur et une caisse coop, est drôle pendant 20 minutes ? »*

### Contenu
| Élément | Détail |
|---|---|
| **Zone** | Une crique du Lagon de ~80 × 80 m, profondeur 5–20 m, un courant, une source de bulles. |
| **Objets** | 150 objets physiques de 6 types (canette, bouteille, pneu, ferraille, caddie, vase précieux) + 2 objets géants (pédalo-cygne, coffre). |
| **Outil de ramassage** | Pince (attraper, lancer). |
| **Aspirateur** | Aspirateur industriel à 2 postes : porte-buse + opérateur de puissance (1–3), avec risque d'aspirer un collègue et bouchon. |
| **Station de vente** | Comptoir avec trémie, compteur de crédits partagé. |
| **Convoyeur** | Un seul convoyeur posé (non constructible) entre la cuve de l'aspirateur et le Comptoir ; peut se bloquer (objet trop gros). |
| **5 améliorations** | 1) Capacité du sac · 2) Portée de pince · 3) Puissance max de l'aspirateur (niveau 4–5) · 4) Vitesse du convoyeur · 5) Ballons de flottabilité (débloque le portage solo plus facile des objets lourds). |
| **Machine 2 joueurs** | L'aspirateur industriel. |
| **Caisse coop** | Caisse lourde à 2 poignées qui coule si portée seul ; contient les objets précieux ; s'ouvre si secouée. |
| **Objectif** | Vider la crique à 100 % → écran de fin avec temps, stats drôles, et zone qui « redevient belle ». |

**Hors-scope prototype :** construction libre, tri, robots, événements (un seul événement scripté éventuellement : le poulpe voleur), oxygène (désactivé), sauvegarde.

### Questions à valider en playtest
1. Le ramassage manuel est-il satisfaisant (feel de la pince, sons) ?
2. L'aspirateur à 2 postes crée-t-il de la communication et des rires ?
3. Le % de propreté donne-t-il envie de finir ?
4. Les 5 upgrades donnent-ils un vrai sentiment de puissance ?
5. La réplication tient-elle avec 4 joueurs et 150 objets à 100 ms de latence ?

**Métriques :** temps pour vider la zone (cible 15–25 min à 2–4 joueurs), nombre de « rires » observés, % de joueurs voulant rejouer, fps et bande passante.

### Équipe et durée
2 à 3 personnes (1 programmeur gameplay/réseau, 1 artiste 3D/tech-art, 1 designer à mi-temps) · **8 à 10 semaines**.
- S1–2 : nage, pince, objets physiques, réseau de base.
- S3–4 : système dormant/actif, Comptoir, crédits.
- S5–6 : aspirateur 2 postes, convoyeur, caisse coop.
- S7–8 : upgrades, % de propreté, feedback (sons, particules).
- S9–10 : playtests, itération, build pour testeurs externes.

---

## 16. Roadmap de développement

| Phase | Durée | Contenu | Jalon |
|---|---|---|---|
| **0. Prototype** | 2–3 mois | MVP ci-dessus | Go/No-Go sur le fun |
| **1. Vertical slice** | 4–5 mois | Lagon complet + Cimetière des bateaux, construction de convoyeurs, trieuse, compacteur, grue 3 postes, 6 événements, DA finale | Trailer + page Steam + démo festival (Steam Next Fest) |
| **2. Production** | 8–10 mois | Zones 3 à 5, robots, drones, sous-marin industriel, économie complète, sauvegarde, contrats | Accès anticipé (Early Access) avec 4 zones |
| **3. Early Access** | 6–9 mois | Zone 6 (Cité), outils absurdes, 22 événements, équilibrage via télémétrie, retours communauté | Mises à jour mensuelles |
| **4. Version 1.0** | — | Polish, localisation (FR/EN/ES/DE/PT-BR/JP), accessibilité, succès Steam | Sortie 1.0 |
| **Post-launch** | — | Nouvelles zones (récif polaire ?), mode « contrat hebdo », workshop cosmétique, portage consoles | — |

**Total jusqu'à l'Early Access :** ~14–18 mois pour une équipe de 4–6 personnes.

**Stratégie de communication :** le jeu est extrêmement « clippable » (collègues aspirés, ancres qui s'envolent). Prioriser les GIF/vidéos courtes dès la vertical slice, et viser les créateurs de contenu coop.

---

## 17. Risques de conception et solutions

| Risque | Impact | Solution |
|---|---|---|
| **Synchronisation réseau des objets physiques** | Bugs, désynchros, lag | Modèle dormant/actif/critique, convoyeurs logiques, flux virtualisés (§14). Tester la latence dès la semaine 2. |
| **Performance avec des centaines d'objets** | fps bas | ISM, plafond d'actifs, LOD physique, culling agressif. |
| **Navigation 3D sous-marine désorientante** | Nausée, frustration | Caméra stabilisée, « haut » toujours clair (lumière de surface), boussole et ping vers la base, nage arcade réactive. |
| **Le solo est ennuyeux** | Perte d'une part du public | Robot Bob pour les postes, objets géants faisables avec ballons, équilibrage spécifique solo. |
| **Un joueur « portait » les autres / déséquilibre de rôles** | Ennui pour certains | Postes courts et tournants, chaque rôle a un feedback fort, aucun rôle passif (même le guide a des outils). |
| **Trolling entre amis (aspirer les autres)** | Frustration | C'est une **feature** mais bornée : K.O. bref, réapparition rapide, option « pas de friendly suck ». |
| **Monotonie du ramassage** | Abandon après 2 h | Nouveau verbe toutes les ~45 min, zones à gimmicks, événements, objets géants. |
| **Automatisation qui supprime le jeu** | Les joueurs regardent l'usine | Les objets géants, trésors et crises restent humains ; l'automatisation libère du temps pour l'épique. |
| **Complexité de l'usine qui fait fuir le public casual** | Barrière | Construction sur grille simple, snapping, presets de chaînes, pas de gestion de ratios complexes. |
| **Physique chaotique qui casse le jeu** (objets coincés, perdus) | Blocage | Tapis de récupération, bouton « débloquer » sur chaque machine, objets hors-limite téléportés. |
| **Scope trop large** | Projet jamais fini | Prototype strict, Early Access avec 4 zones, outils absurdes en post-lancement si besoin. |
| **Ressemblance trop forte avec les jeux de référence** | Manque d'identité | Identité forte : 3D verticale, flottabilité, machines à postes, univers d'entreprise satirique. |
| **Événements frustrants** | Rage-quit | Annonce préalable, durée courte, toujours une récompense, réglables dans les options. |

---

## Annexe — Résumé en une page

- **Quoi :** jeu coop 1–4 joueurs, vider des zones sous-marines puis industrialiser la récupération.
- **Pourquoi c'est fun :** le tas qui fond, la montée en puissance, les machines à postes, la physique flottante cartoon, les accidents entre amis.
- **Ce qui le rend unique :** flottabilité comme outil coop, nettoyage en 3D, chaîne industrielle physique qui peut s'effondrer, objets géants qui exigent de s'organiser.
- **Comment le faire :** UE5, listen server, simulation physique à 3 niveaux, convoyeurs logiques, prototype de 10 semaines pour valider le fun avant tout le reste.
