# Slider

Un fader dont la position est une valeur de contrôle que n'importe quel bouton rotatif du patch peut suivre.

La sortie porte la position de la poignée comme valeur de contrôle : un bouton rotatif peut la suivre par un câble de contrôle ou un routage Contrôler avec, et l'entrée règle la poignée depuis un autre contrôle, pour qu'un capteur ou un LFO puisse la déplacer. Min et Max sont les nombres que représentent les deux extrémités, et la sortie déclare cette plage : un bouton rotatif suit la position sur toute sa propre course, tandis qu'un Number câblé sur la même sortie lit la valeur convertie. Assigne un fader matériel à Value par l'apprentissage MIDI et il devient une macro qui pilote plusieurs routages à la fois. Donne une forme d'interrupteur à un routage et le Slider bascule un Bypass ou ouvre un Gate au-delà d'un seuil.

## Paramètres

**Value** La poignée, de 0 à 1. L'entrée la règle et la sortie l'envoie.

**Min** Le nombre que représente le bas de la course. Min au-dessus de Max inverse la course.

**Max** Le nombre que représente le haut de la course.

**Slew** Le temps que met la valeur à atteindre un nouveau réglage, en millisecondes. À 0, elle arrive tout de suite ; plus haut, un saut devient un glissement.

**Log** Courbe la course entre Min et Max de façon logarithmique, pour donner plus de place au bas. Utilise-le pour les fréquences et les durées ; les deux extrémités doivent être au-dessus de zéro.

## Recette

**Macro de filtre** Min 80, Max 12000, Log activé, Slew 60. Fais un clic droit sur le Cutoff d'un Filter et choisis Contrôler avec Slider, puis fais un clic droit sur le Feedback d'un Fern et fais de même avec une plage plus étroite. Un seul fader ouvre maintenant le filtre et allonge l'écho en même temps ; assigne Value à un fader matériel par l'apprentissage MIDI.

## Organisms associés

Number, Sig, LFO, Gate
