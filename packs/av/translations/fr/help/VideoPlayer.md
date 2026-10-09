# VideoPlayer

Une platine vidéo qui fait tourner une cassette en boucle dans la chaîne vidéo.

Charge une cassette dans l'emplacement File, ou dépose dessus un clip de la timeline, et câble la sortie vidéo vers un calque de Lumen, un VideoFX, un VideoMix ou un VideoOut ; la cassette y joue, en boucle. Rate est le moteur, et Opacity et Blend règlent la façon dont le calque se compose, tous des paramètres ordinaires, donc un Follower peut piloter la vitesse avec la musique. L'entrée MIDI est un déclencheur de relance : chaque note-on rembobine et relance la cassette, donc une platine par couloir de lancement, alimentée par une Sequence ou un PianoRoll, coupe la vidéo en rythme. La platine tourne tant que son éditeur est ouvert ou que quelque chose en aval regarde sa sortie, et se repose sinon.

## Paramètres

**File** La vidéo à lire.

**Follow** Prend la position de l'image dans Position au lieu de jouer à sa propre vitesse. Câble la sortie de position d'un Deck dans Position et la vidéo suit le disque : scratche-le, pousse-le, rembobine-le, et l'image suit la main. Désactivé, le fichier joue tout seul à la vitesse Rate.

**Position** L'endroit de la vidéo à montrer, de son début à sa fin, quand Follow est activé.

**Rate** Le bouton rotatif Speed, en pourcentage. 100 % est la vitesse d'origine, 0 % tient une image fixe, 200 % double la vitesse.

**Opacity** L'intensité avec laquelle ce calque apparaît par-dessus ceux du dessous dans un Lumen.

**Blend** La façon dont il se combine avec ces calques : Normal, Add, Multiply ou Screen.

## Recette

**Boucle coupée par les notes** Charge une cassette courte, câble la sortie vers un VideoOut et une Sequence dans l'entrée MIDI avec une note par mesure, pour que la cassette redémarre sur le premier temps. Mets un LFO lent et peu profond sur Rate autour de 100 % et la boucle respire contre la grille.

## Organisms associés

Lumen, VideoMix, VideoOut
