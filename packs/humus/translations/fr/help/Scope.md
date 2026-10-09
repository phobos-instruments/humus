# Scope

Un oscilloscope à la trace verte lumineuse, dans la tradition de l'appareil de mesure d'atelier. Il ne fait qu'écouter : il n'a pas de sortie, alors tire un deuxième câble jusqu'à lui depuis ce que tu veux voir.

L'écran montre l'onde elle-même : le temps défile horizontalement, le niveau monte verticalement, et la ligne centrale est le silence. Chaque balayage démarre là où l'onde remonte en traversant la ligne centrale, donc une hauteur stable reste immobile au lieu de défiler. La trace laisse une courte rémanence, et elle est plus brillante là où le faisceau bouge lentement et plus pâle sur un front raide : une onde carrée montre donc des paliers brillants reliés par des montées pâles. Un câble branché seulement sur l'entrée 1 s'affiche sur les deux canaux.

## Paramètres

**Mode** STEREO dessine le canal gauche en vert et le droit en vert plus pâle sur le même écran. MONO dessine leur somme. X/Y pousse le faisceau horizontalement avec le canal gauche et verticalement avec le droit : un signal mono est une ligne diagonale, un signal large s'ouvre en nuage, et deux oscillateurs désaccordés dessinent des nœuds qui tournent lentement.

**Hold** Arrête l'écran là où il est pour que tu puisses le lire.

**Time** Le temps affiché sur la largeur de l'écran, de 1 ms à une demi-seconde. Les temps courts montrent la forme d'un cycle ; les longs montrent l'enveloppe d'une note.

**Zoom** Agrandit la trace, jusqu'à huit fois, pour les signaux faibles. Il ne change que l'image, jamais le son.

## Recette

**Voir ce que fait un filtre** Relie un Wave qui joue une dent de scie à un Filter par un câble, et le Filter au Scope. Avec Time vers 10 ms, les dents pointues s'arrondissent quand la fréquence de coupure descend, et monter la résonance dessine le tintement sur chaque front.

**Vérifier l'image stéréo** Relie aussi à lui ce qui alimente le SoundOut, et passe en X/Y. Une ligne penchée vers la droite est un mono sain, une ligne penchée vers la gauche veut dire qu'un canal est inversé, et un nuage rond est une stéréo large qui peut s'amincir sur une seule enceinte.

## Organisms associés

Spectrum, VuMeter, StereoTool, Wave
