# Harmonizer

Un harmoniseur à suivi de hauteur qui ajoute jusqu'à quatre copies transposées d'une seule ligne mélodique.

Il écoute la hauteur à son entrée et construit des harmonies autour, chacune étant la même interprétation transposée sur une nouvelle note. Les harmonies ne sonnent que quand une hauteur claire est entendue, donc les respirations et les consonnes passent proprement. En mode MIDI, les notes tenues à l'entrée MIDI fixent les cibles : chante une note et l'accord suit ta main. En mode SCALE, Key, Scale et les quatre intervalles Voice les fixent, donc les harmonies restent dans la tonalité où que la mélodie aille. Branche-y un SoundIn ou un FilePlayer, et un PianoRoll ou un clavier sur l'entrée MIDI pour le mode MIDI.

## Paramètres

**Mode** MIDI prend les cibles dans les notes tenues à l'entrée MIDI, jusqu'à quatre, à plein niveau. SCALE les déduit de Key, Scale et des intervalles Voice.

**Key** La tonalité de référence du mode SCALE, de C à B.

**Scale** La gamme dans laquelle comptent les intervalles, de Chromatic et Major jusqu'à Blues, Whole Tone et Hirajoshi.

**Voice1** L'intervalle de la première voix en degrés de la gamme, de -7 à 7 : 2 est une tierce au-dessus, -3 une quarte en dessous, 7 l'octave. Pareil pour Voice2 à Voice4.

**Level1** Le niveau de la première voix en mode SCALE ; une voix à zéro se tait. Pareil pour Level2 à Level4.

**Glide** Le temps que met une voix à atteindre une nouvelle cible, en millisecondes. 0 saute directement.

**Humanize** Une lente dérive de désaccord propre à chaque voix, jusqu'à environ 14 cents, pour que l'empilement cesse de sonner comme une seule machine.

**Spread** Déploie les voix dans le champ stéréo autour de la ligne directe.

**Mix** Fondu de la ligne directe seule aux harmonies seules. Tout en signal traité, c'est le silence partout où aucune harmonie ne sonne.

## Recette

**Tierces diatoniques** Mode SCALE, Key sur la tonalité du morceau, Scale Major, Voice1 2 avec Level1 0.8, Voice3 -3 avec Level3 0.5, Glide 40, Humanize 0.2, Spread 0.6, Mix 0.5. Fais entrer une voix chantée depuis un SoundIn, et place un Compressor après pour souder l'empilement.

## Organisms associés

Cluster, Prism, Trellis, Decomposer
