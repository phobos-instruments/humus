# Spectrum

Un analyseur de spectre en direct avec une cascade. Il ne fait qu'écouter : il n'a pas de sortie, alors tire vers lui un second câble depuis ce que tu veux lire.

Le panneau du haut montre le spectre à l'instant, avec une fine ligne de maintien des crêtes qui garde en mémoire le niveau le plus fort atteint par chaque bande. Le panneau du bas est la cascade, le même spectre dessiné dans le temps, avec la rangée la plus récente en haut. La fréquence suit une échelle logarithmique de 30 Hz à 18 kHz et le niveau monte à partir d'un plancher de -72 dB. Les deux canaux sont sommés pour l'affichage, et dans le silence l'affichage garde sa dernière image. Câble-y la sortie d'un Filter pour voir un balayage, ou ce qui alimente le SoundOut pour lire tout le mix.

## Recette

**Lire un mix** Câble le Mixer vers lui en plus du SoundOut. Deux sons qui se battent apparaissent comme des crêtes au même endroit, le souffle comme un plancher qui ne s'assombrit jamais, et une résonance dans un Filter comme un pic qui bouge. Mets les canaux du Mixer en solo un par un en regardant, pour trouver à qui appartient une crête.

## Organisms associés

Scope, VuMeter, StereoTool, Prism, SpectralFilter
