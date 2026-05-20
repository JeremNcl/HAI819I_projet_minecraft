# Roadmap & Stratégie Révisée - Moteur de Jeu Voxel (Architecture 100% ECS)

## Version 3.0 - Cycle de 5 Jours de Code + 1 Jour de Livrables

Ce document planifie le développement complet du prototype de moteur de jeu voxel en optimisant la notation selon la grille d'évaluation de l'UE HAI8191. Il acte l'abandon du Graphe de Scène classique au profit d'une architecture purement Data-Oriented (ECS). Le travail est réparti entre un binôme principal (accès Git) et un membre bonus (boîte noire algorithmique).

---

## 🛑 1. Nettoyage de l'Architecture : Fichiers à Supprimer / Ignorer

Puisque nous passons sur un ECS pur, les classes Orientées Objet traditionnelles n'ont plus lieu d'être. Vous devez supprimer :

* **Dossier `common/terrain/` (À supprimer entièrement) :**
    * `heightmapLoader.cpp/hpp`, `terrain.cpp/hpp` (Incompatibles avec les voxels).
    * `terrainNode.cpp/hpp` et tout autre héritage de `SceneNode`.
* **Le Graphe de Scène (À archiver/supprimer) :**
    * `SceneGraph`, `SceneNode` : Le rendu sera désormais géré par un `RenderSystem` itérant sur des composants `MeshComponent` et `Transform`.
* **Dans le dossier `TP1/` :**
    * `heightmaps/` et `meshes/` (Les `.off` ne sont plus la base du monde).
* **Dans `mesh.cpp/hpp` :**
    * Conservez la gestion des VBO/VAO (`vertices`, `indices`), mais supprimez `loadFromOFF`, `computeNormals()`, etc.

---

## 📊 2. Grille d'Évaluation & Objectifs de Score (Objectif 20/20)

| Critère d'évaluation | Points | Objectif de Performance Visé (Niveau A) | Implémentation Spécifique ECS |
| :--- | :--- | :--- | :--- |
| **Bases essentielles** | 2 pts | Architecture robuste, sans bug, gestion optimale de la mémoire. | Implémentation d'un ECS maison basé sur des **Sparse Sets**. Création des composants de base (`Transform`, `Mesh`, `Camera`). |
| **Interactions** | 2 pts | Interaction poussée avec le monde et lien direct avec le HUD. | `PlayerActionSystem` gérant le Raycasting 3D pour cibler, détruire et poser des blocs. HUD géré par un composant d'interface. |
| **Physique** | 2 pts | Intégralement programmé, gestion fluide des mouvements et collisions. | `PhysicsSystem` manipulant `Transform`, `Velocity` et `ColliderAABB`. Résolution des collisions contre les Chunks. |
| **Rendu** | 2 pts | Rendu PBR Cook-Torrance complet sans bug flagrant. | `RenderSystem` qui itère sur toutes les entités possédant un `Transform` et un `Mesh`, appliquant les shaders PBR. |
| **Spécialisation** | 5 pts | Fonctionnalités complexes hors-norme débloquant des limites importantes. | **Triple spécialisation :** <br>1. Données Voxel en Chunks (16x256x16). <br>2. `CullingSystem` (Frustum Culling). <br>3. `AISystem` (Pathfinding A* 3D). |
| **Soutenance** | 5 pts | Démo convaincante, respect strict des temps, maîtrise technique. | Session intensive le Jour 6 pour livrer une vidéo parfaite et justifier le choix du Data-Oriented Design (ECS). |

---

## 📅 3. Le Planning Révisé (6 Jours)

### 🛠️ Jour 1 : Cœur ECS, Chunks & Caméra FPS
*Mise en place de l'infrastructure logicielle et des premières données.*

* **Dév 1 (Cœur ECS & Données Voxel) :** Implémentation de la classe `Registry` et des `SparseSets`. Création du composant `VoxelData` (tableau 1D aplati des blocs). Écriture du **ChunkMeshingSystem** : lit les `VoxelData` et génère/met à jour un composant `MeshComponent` (seulement les faces visibles).
* **Dév 2 (Entité Joueur & Inputs) :** Création de l'entité `Player` dans l'ECS avec les composants `Transform`, `Camera` et `InputReceiver`. Création du `CameraSystem` (met à jour les matrices de vue) et du `InputSystem` (capture GLFW et met à jour les intentions de mouvement).
* **Membre Bonus (Générateur Procédural) :** Classe `TerrainGenerator` isolée (bruit de Perlin/Simplex). *Input :* Coordonnées du Chunk. *Output :* Tableau de types de blocs bruts.

### 🏃‍♂️ Jour 2 : Physique, Collisions & Frustum Culling
*La logique de mouvement entre en jeu de manière découplée.*

* **Dév 1 (PhysicsSystem) :** Création des composants `Velocity` et `ColliderAABB`. Le `PhysicsSystem` itère sur ces composants : applique la gravité, teste les intersections avec les entités possédant un `VoxelData`, et corrige le `Transform` pour éviter de traverser le sol ou permettre les sauts.
* **Dév 2 (CullingSystem & RenderSystem) :** Création du `RenderSystem` chargé d'appeler OpenGL pour les entités ayant un `MeshComponent`. Création du `CullingSystem` qui calcule les 6 plans de la caméra et désactive le rendu des entités (Chunks) hors du champ de vision.
* **Membre Bonus (Pathfinder A* 3D) :** Classe autonome `Pathfinder3D`. *Input :* Point A, Point B, accès aux tableaux `VoxelData`. *Output :* Liste de coordonnées.

### 🎨 Jour 3 : Rendu PBR & Interaction Joueur
*Le moteur devient beau et le joueur peut modifier le monde.*

* **Dév 1 (PlayerActionSystem) :** Ajout du lancer de rayon (Raycasting). Lorsqu'un clic est détecté, le système identifie le voxel ciblé. S'il est modifié (détruit/posé), le `ChunkMeshingSystem` est alerté pour recalculer le `MeshComponent` de ce Chunk spécifique.
* **Dév 2 (Shaders PBR Cook-Torrance) :** Intégration du modèle PBR dans le `RenderSystem`. Configuration des paramètres (Rugosité, Métallique) pour les différents types de blocs (Terre, Roche) dans les shaders GLSL.
* **Membre Bonus (Décorations) :** Extension du générateur pour y inclure des structures multi-blocs (arbres, ruines) lors de la création initiale du terrain.

### 🤝 Jour 4 : L'IA et l'Intégration du Monde
*Les systèmes interagissent ensemble et le monde s'étend.*

* **Dév 1 & Dév 2 (Monde infini & Génération) :** Branchement du générateur du membre bonus. Mise en place d'un système qui crée de nouvelles Entités "Chunk" dynamiquement autour du joueur lorsqu'il se déplace.
* **Dév 1 (AISystem & Monstres) :** Création des Entités Monstres. Elles reçoivent `Transform`, `Velocity`, `ColliderAABB`, `MeshComponent` **et** un nouveau `AIComponent`. L'`AISystem` interroge le `Pathfinder3D` du membre bonus et modifie la `Velocity` du monstre. Le `PhysicsSystem` (déjà codé au Jour 2) s'occupera de les faire bouger sans traverser les murs !
* **Dév 2 (Optimisations) :** Mesures de performance. Vérification de la contiguïté mémoire des Sparse Sets et optimisation des requêtes de composants.

### 💎 Jour 5 : Stabilisation & Polish
*Gel des fonctionnalités, on rend le jeu présentable.*

* **Dév 1 (Gameplay) :** Ajustement des constantes (friction, vitesse de déplacement, réactivité des sauts). Résolution des bugs de collision aux frontières entre deux Chunks.
* **Dév 2 (Ambiance Visuelle) :** Ajout d'une entité Skybox. Réglage fin de la lumière directionnelle (Soleil) passant à travers les Shaders PBR. Ajout d'un crosshair.
* **Membre Bonus (Pré-production Soutenance) :** Rédaction des brouillons pour justifier l'architecture ECS. Création de schémas montrant comment les Systèmes interagissent avec les Composants.

---

## 🎬 4. Jour 6 : Livrables & Soutenance (Zéro Code)

Journée sanctuarisée pour la validation académique.

1. **Enregistrement Vidéo :**
    * Gameplay : Déplacements fluides, destruction de blocs, IA en action.
    * Technique : Affichage des FPS et du nombre de drawcalls avec/sans le `CullingSystem` activé.
2. **Rapport :**
    * Argumentaire fort sur le choix de l'ECS (Data Locality, découplage de la physique et du rendu).
    * Preuves par l'image des critères PBR, Raycasting et Pathfinding.
3. **Soutenance (10 min) :**
    * Répétition chronométrée.
    * Mise en avant des trois spécialisations.
    * Préparation aux questions de jury sur le fonctionnement interne de vos Sparse Sets (le Swap & Pop).
