# Microdot

Un synthé de basse roulante à contretemps, piloté par une grille de déclenchement à seize pas calée sur le transport.

Chaque coup est un oscillateur à bande limitée, avec en option un carré à l'octave inférieure, passé dans une enveloppe de filtre rapide et un étage de saturation. Un organism tout neuf démarre avec le roulement classique, trois doubles croches après chaque temps, si bien qu'il se cale dès que le transport joue, et il reste silencieux à l'arrêt. La grille fait toujours office de gate ; Seq décide à qui appartient la hauteur. Avec Seq activé, chaque coup joue le champ Note. Avec Seq désactivé, les coups suivent ce que tient l'entrée MIDI, si bien qu'un Riff, un Steps ou un PianoRoll câblé en entrée voit sa ligne redécoupée par la grille. Associe-le à un Kick en 4/4 et câble les deux vers un Compressor.

## La grille

Clique sur un pas pour l'activer ou le désactiver ; la colonne où se trouve le transport est éclairée. La rangée ^ monte le coup de ce pas d'une octave, et la rangée de tenue en bas prolonge la note précédente sur ce pas, si bien qu'une suite de tenues maintient une seule note. Fais un clic droit pour les motifs tout faits : Roulement -111, Contretemps --1-, Plein 1111, Aléatoire et Effacer. Le motif est enregistré dans le patch.

## Paramètres

**Note** La hauteur que joue chaque coup quand Seq est activé.

**Wave** SAW, SQR, PLS (une impulsion fine), TRI, SIN ou FAT, une paire de dents de scie désaccordées pour la largeur.

**Seq** Activé, c'est le champ Note qui détient la hauteur ; désactivé, c'est la note MIDI tenue. La grille sert de gate dans les deux cas.

**Cutoff** La fréquence de repos du filtre.

**Reso** Résonance. Monte-la et le balayage de l'enveloppe se met à chanter.

**EnvMod** De combien l'enveloppe ouvre le filtre au-dessus de Cutoff à chaque coup.

**Decay** Longueur de l'enveloppe de filtre en millisecondes ; elle façonne aussi le niveau de la note. Entre 80 et 150, tu gardes le caractère roulant et serré.

**Gate** Durée du coup, en fraction de pas. Plus court, c'est plus rebondissant.

**Sub** Un carré à l'octave inférieure en dessous, pour du poids.

**Drive** Saturation après le filtre.

**Level** Niveau de sortie.

**SwingFollow** Follow : prend le groove du patch sur le transport au lieu du curseur Swing.

**Swing** Retarde une double croche sur deux, jusqu'à une sensation de triolet. Utilisé seulement quand Follow est désactivé.

## Recette

**Basse roulante** Note A1, Wave SAW, Cutoff 700, Reso 0.35, EnvMod 0.5, Decay 120, Gate 0.8, Sub 0.4, Drive 0.35, Follow activé. Ajoute un Kick avec 4/4 activé et appuie sur Lire ; la grosse caisse tombe sur le temps et la basse roule entre les deux. Désactive Seq et câble un Riff en entrée pour donner au roulement une ligne qui bouge.

## Organisms associés

Kick, Acid, Riff, DNA
