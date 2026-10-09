# VideoFX

Un traitement d'image avec des réglages de position, d'échelle, de rotation, de couleur et de mosaïque.

Une entrée vidéo et une sortie. L'image passe par un étage de transformation (position, échelle, rotation, miroir et pixellisation) puis par un étage de couleur (luminosité, contraste, saturation, teinte et inversion). Chaque bouton rotatif est un paramètre ordinaire : un Follower sur Scale pousse l'image au rythme du kick, un LFO sur Rotate la fait tourner lentement et une Sequence sur Hue fait défiler les couleurs en mesure. Place-le entre un VideoPlayer, un VideoPad ou une CameraIn et un Lumen, un VideoMix ou un VideoOut, et enchaînes-en deux quand la transformation et la couleur doivent s'automatiser séparément.

## Paramètres

**PosX** Où se place l'image de gauche à droite, en proportion du cadre. +100% la décale d'un cadre entier vers la droite.

**PosY** Pareil de haut en bas. +100% correspond à un cadre entier vers le haut.

**Scale** La taille. 100% la laisse telle quelle, 200% la double, 0% la réduit à un point.

**Rotate** La rotation en degrés autour du centre du cadre.

**Pixelate** La grossièreté de la mosaïque. 0% laisse l'image intacte ; 100% ne laisse qu'une poignée de blocs en largeur.

**Mirror** Replie le cadre sur son milieu. Off le laisse entier, Left copie la moitié gauche sur la droite, Top copie la moitié du haut sur le bas, Both fait les deux.

**Brightness** La lumière. 100% la laisse telle quelle, 0% donne du noir, 200% double la luminosité.

**Contrast** L'écart autour du gris moyen. 0% aplatit tout en gris, 200% pousse vers les extrêmes.

**Saturation** Le bouton de couleur. 0% donne du monochrome, 100% laisse tel quel, 200% sature à l'excès.

**Hue** Fait tourner chaque couleur sur le cercle chromatique, en degrés.

**Invert** Affiche le négatif.

## Recette

**Zoom sur le kick** Relie un VideoPlayer à VideoFX et VideoFX à un VideoOut. Relie le kick à un Follower avec un relâchement court et envoie le Follower vers Scale avec une petite profondeur au-dessus de 100%, pour que chaque coup pousse l'image vers toi et qu'elle revienne entre deux coups. Ajoute une pointe de Pixelate pour un rendu plus brut.

## Organisms associés

VideoPlayer, VideoPad, VideoMix, Lumen
