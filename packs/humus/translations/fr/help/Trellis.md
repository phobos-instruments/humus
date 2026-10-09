# Trellis

Un correcteur de hauteur monophonique qui ramène une voix ou une ligne lead sur les notes d'une tonalité et d'une gamme.

Il suit la fondamentale entrante, la cale sur la note autorisée la plus proche dans la Key et la Scale choisies, et décale l'audio jusque-là avec un pitch shifter à ligne de retard en fondu enchaîné, une voix à la fois. Speed règle la vitesse du glissement vers la cible : lent garde la dérive naturelle, rapide donne l'effet hard-tune. La grille de notes vient de l'accordage du patch, donc un organism Tuning réglé en 19-EDO pose les masques de gamme sur 19 degrés au lieu de 12. Le chemin du signal traité porte un court retard. Câble en entrée une voix mono venant d'un SoundIn, ou un Decomposer.

## Paramètres

**Strength** À quel point la hauteur est tirée vers la cible. 0 laisse l'interprétation intacte ; 1 la cale entièrement sur la grille.

**Speed** La vitesse à laquelle la hauteur glisse vers la cible. Bas, c'est une correction transparente ; près du maximum, c'est le calage robotique.

**Key** La tonique, de C à B. Clique sur la case et appuie sur une touche, sur un clavier MIDI ou sur le piano qui s'ouvre ; n'importe quelle octave donne la même tonique.

**Scale** Les notes sur lesquelles la hauteur peut tomber : Chromatic, Major, Minor, les modes, Harmonic et Melodic Minor, les deux pentatoniques, Blues, Whole Tone et Hirajoshi.

**Mix** La quantité de signal corrigé entendue face au signal direct.

## Recette

**Correction naturelle** Key sur la tonalité du morceau, Scale Major ou Minor, Strength 0.7, Speed 0.4, Mix 1. Pour l'effet à la place : Scale Chromatic, Strength 1, Speed 1. Câble la sortie vers un Verbatim ou un Fern pour que la correction se pose dans un espace.

## Organisms associés

Decomposer, Tuning
