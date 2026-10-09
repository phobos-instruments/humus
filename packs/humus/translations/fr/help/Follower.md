# Follower

Un suiveur d'enveloppe qui transforme le volume d'un signal en valeur de contrôle et en gate.

Il redresse l'audio sur son entrée et le lisse en un contour, publié comme valeur de contrôle "env" sur la première sortie, de 0 à 1 environ pour un audio à pleine échelle, et mis à l'échelle par Gain. Une deuxième sortie et la valeur de contrôle "gate" restent à 0 jusqu'à ce que le contour franchisse Threshold, puis tiennent à 1 jusqu'à ce que le niveau soit retombé sous la moitié du seuil et que Hold soit écoulé. Chaque ouverture envoie aussi Note par la sortie MIDI, avec une vélocité tirée du niveau du coup. Branche env sur un Gain pour un ducking ou un expandeur, sur le Frequency d'un Filter pour un auto-wah, ou la sortie MIDI dans un VideoPad pour couper les images sur le temps. Pour une dynamique toute faite dans un seul organism, regarde SideChain et la famille Compressor.

## Paramètres

**Attack** La vitesse à laquelle le contour monte sur un coup, en millisecondes. Court, il attrape les transitoires ; long, il les lisse en un niveau.

**Release** La lenteur avec laquelle le contour redescend ensuite, ce qui décide combien de temps un son garde le gate ouvert une fois qu'il s'arrête.

**Gain** Met le contour à l'échelle pour que sa course corresponde à ce qu'il pilote.

**Threshold** Le niveau que le contour doit atteindre pour ouvrir le gate. 0 désactive le gate.

**Hold** Le temps minimum pendant lequel le gate reste ouvert une fois qu'un coup l'a ouvert, en millisecondes, pour qu'un seul coup de batterie ne se morcelle pas en plusieurs notes.

**Note** La note MIDI envoyée à chaque ouverture du gate.

## Recette

**Coupes au rythme de la grosse caisse** Branche une grosse caisse dans le Follower, Attack 2, Release 150, Threshold 0.3, Hold 100. Branche sa sortie MIDI dans un VideoPad et chaque grosse caisse déclenche le pad ; branche en même temps env sur le Gain d'un synthé de nappe et le synthé s'efface sous chaque coup.

## Organisms associés

Gain, SideChain, Filter, Button
