# TFA_Read-toon

Projet pour ESP32 utilisant un écran TFT et une carte SD pour explorer, afficher et manipuler des fichiers et images.

## Fonctionnalités

- Navigation dans l'arborescence de la carte SD avec un encodeur rotatif.
- Affichage des fichiers et dossiers sur un écran TFT.
- Affichage d'images JPEG.
- Lecture de fichiers texte.
- Mode SCAN pour visualiser des images en séquence.
- Interface graphique personnalisée avec options contextuelles.

## Dépendances

- [PlatformIO](https://platformio.org/)
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- [JPEGDecoder](https://github.com/Bodmer/JPEGDecoder)
- Arduino framework pour ESP32

## Structure

- `src/` : Code principal du projet.
- `lib/Graphics/` : Fonctions graphiques pour l'affichage.
- `lib/File_essantial/` : Structures et utilitaires pour la gestion de fichiers.
- `platformio.ini` : Configuration PlatformIO.

## Utilisation

1. Cloner ce dépôt.
2. Ouvrir le dossier avec PlatformIO (VSCode).
3. Connecter un ESP32 avec écran TFT et carte SD selon le schéma matériel.
4. Compiler et téléverser le projet.
5. Utiliser l'encodeur pour naviguer dans les fichiers.

## Auteurs

- Marius

## Licence

Ce projet est sous licence MIT.
