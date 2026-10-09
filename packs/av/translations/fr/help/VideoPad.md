# VideoPad

Huit pads de clips avec un fondu enchaîné entre les lancements.

Chaque pad contient un clip : dépose une vidéo dessus ou clique sur le dossier pour en charger une, clique sur l'image pour le lancer, et le passage à ce clip se fait en fondu enchaîné sur la durée de Fade. L'entrée MIDI lance les pads par note, le do central pour le pad 1 et les demi-tons au-dessus pour les autres ; lancer un pad qui joue déjà le fait repartir de In. Aléatoire tire de nouveaux In, Out et Loop pour chaque pad chargé et en lance un. Câble la sortie principale vers un calque de Lumen, un VideoFX, un VideoMix ou un VideoOut.


Les pads portent leur son en plus de leur image. Un clip lancé joue son propre audio par la sortie stéréo, depuis son point In, en boucle entre In et Out si ce pad boucle, et passe la main au pad suivant avec le même fondu que l'image. Mute coupe les deux. L'image suit le son au lieu de courir à côté, donc une coupe tombe sur les deux en même temps au lieu de se décaler au fil d'un long clip - et un pad pressé alors qu'il joue déjà revient à son point In, son et image ensemble.

## Édition des pads

La ligne du haut sous chaque image représente toute la bande, avec la portion de In à Out éclairée ; la ligne du bas représente la coupe, avec sa tête de lecture. Fais glisser une marque pour déplacer un point, fais glisser sur l'image pour la parcourir, et appuie sur In ou Out pour inscrire la position de la tête de lecture dans le pad. Fais glisser un pad par son badge numéroté sur un autre pad pour le copier, ou sur la timeline pour en faire un clip.

## Paramètres

**File1** Le clip du pad 1, et ainsi de suite de 2 à 8.

**Launch1** Lance le pad 1. Momentané ; fais un clic droit sur un pad et choisis Contrôle du lancement pour l'assigner, et ainsi de suite de 2 à 8.

**In1** Où commence le clip du pad 1, en secondes. Tiré par Aléatoire, et ainsi de suite de 2 à 8.

**Out1** Où finit le clip du pad 1, en secondes ; 0 va jusqu'à la fin du fichier. Tiré par Aléatoire, et ainsi de suite de 2 à 8.

**Loop1** Si le pad 1 boucle entre In et Out, ou joue une fois jusqu'au bout et reste sur sa dernière image. Tiré par Aléatoire, et ainsi de suite de 2 à 8.

**Stop** Fait disparaître l'image en fondu au noir sur la durée de Fade.

**Mute** Garde la sortie au noir tant qu'il est activé, un interrupteur de blackout qui laisse les clips tourner en dessous.

**Fade** Durée d'un fondu enchaîné, en secondes. 0 donne une coupe franche.

**Rate** Le bouton rotatif Speed, en pourcentage, pour tous les pads à la fois. 100 % est la vitesse d'origine, 0 % fige une image.

**Pads** 4 pads ou 8 pads.

**Outlets** One out transporte seulement le mélange des lancements. Per pad ajoute une sortie vidéo pour chaque pad après la principale, pour qu'un seul pad puisse alimenter son propre VideoFX ou tracker.

**Opacity** À quel point la sortie principale apparaît quand elle alimente un calque de Lumen.

**Blend** Comment elle s'y combine : Normal, Add, Multiply ou Screen.

## Recette

**Coupes au pad** Charge quatre clips, règle Pads sur 4 pads et Fade sur 0.1. Câble un PianoRoll ou un MidiIn sur l'entrée MIDI et joue le do central et les trois demi-tons au-dessus pour passer d'un clip à l'autre sur le temps. Câble la sortie vers un VideoOut.

## Organisms associés

VideoPlayer, VideoFX, VideoMix, Lumen
