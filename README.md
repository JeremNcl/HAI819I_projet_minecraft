# Voxel Engine - Prototype (Minecraft-like) 🧱

**Projet de l'UE Moteur de Jeux (HAI819I) - Master IMAGINE (Université de Montpellier)**

Ce projet est un prototype de moteur de jeu 3D orienté Voxel, développé en C++ et OpenGL. Il se concentre sur l'optimisation spatiale, la génération dynamique et un rendu physiquement réaliste.

## 🌟 Fonctionnalités Clés et Spécialisations

* **Architecture Voxel & Chunking :** Génération dynamique de maillages optimisés (Meshing naïf : seules les faces exposées à l'air sont dessinées).
* **Spécialisation 1 - Optimisation Spatiale :** Implémentation du *Frustum Culling* pour ne rendre que les chunks et objets présents dans le champ de vision de la caméra.
* **Spécialisation 2 - IA & Pathfinding :** Algorithme de recherche de chemin A* en 3D permettant aux entités de naviguer intelligemment dans l'environnement voxel.
* **Moteur Physique :** Système de collisions AABB personnalisé (gravité, sauts, collisions contre les murs).
* **Rendu Avancé :** Pipeline de rendu PBR (modèle Cook-Torrance) avec éclairage directionnel.
* **Interactions Joueur :** Raycasting 3D pour la destruction et le placement de blocs en temps réel.

## 📁 Architecture du Projet

Le code source respecte une séparation stricte entre le moteur générique (Engine), la logique du jeu (Game) et les algorithmes isolés (Modules) :

```text
├── assets/             # Ressources du jeu (Shaders GLSL, Textures)
├── external/           # Bibliothèques tierces (GLEW, GLFW, GLM)
└── src/                # Code source
    ├── main.cpp        # Point d'entrée et boucle de jeu
    ├── engine/         # Cœur du moteur (Rendu, Graphe de Scène, Caméra, Inputs)
    ├── game/           # Logique spécifique (Chunks, Monde Voxel, Moteur AABB)
    └── modules/        # Boîtes noires algorithmiques (Terrain procédural, Pathfinding A*)
```

## ⚙️ Prérequis et Compilation

Ce projet utilise **CMake** couplé à un **Makefile** pour une compilation simplifiée.

**Dépendances requises :**
* Compilateur C++
* CMake (>= 3.5)
* OpenGL (Core Profile >= 3.3)

**Instructions de build (Linux / macOS) :**

Ouvrez un terminal à la racine du projet et exécutez :

```bash
# 1. Configurer l'environnement CMake (à faire une seule fois)
make configure

# 2. Compiler et lancer le jeu (multithreadé)
make all
```
*(Note : une fois configuré, un simple `make` suffit pour recompiler les fichiers modifiés).*

## 🎮 Contrôles (FPS)

* `W, A, S, D` / `Z, Q, S, D` : Se déplacer
* `Barre d'espace` : Sauter
* `Souris` : Mouvements de caméra
* `Clic Gauche` : Détruire un bloc (Raycasting)
* `Clic Droit` : Poser un bloc
* `Échap` : Quitter le jeu

## 👥 Équipe de Développement

* **Membre 1** - *Architecture système, Rendu & Intégration*
* **Membre 2** - *Physique AABB & Frustum Culling*
* **Bonus** - *Algorithmes de Génération (Bruit) & Pathfinding A**
