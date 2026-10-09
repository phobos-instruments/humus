# Number

Une boîte à nombre qui affiche une valeur en direct, la reçoit d'une entrée de contrôle et la transmet par une sortie de contrôle.

Les câbles portent le son et les nombres atterrissent sur les boutons rotatifs ; un Number est l'endroit où tu regardes un nombre en route vers un bouton. Câble un Follower, un Slider, un RNG ou un autre Number sur son entrée et la boîte montre ce qui arrive, sans mise à l'échelle ; tape une valeur et elle tient jusqu'à ce que quelque chose de nouveau arrive. N'importe quel bouton rotatif du patch peut suivre la sortie : trace un câble de contrôle vers une prise, ou fais un clic droit sur le bouton et choisis Contrôler avec. Comme Value est un paramètre ordinaire, les couloirs d'automation et le contrôle MIDI peuvent aussi le piloter, ce qui fait de Number le pont entre eux et chaque prise du patch.

## Paramètres

**Value** Le nombre que contient la boîte. L'entrée le règle, la saisie le règle, et la sortie le transmet.

**Format** La forme sous laquelle le nombre sort. Any envoie exactement ce que la boîte contient, Whole arrondit à un entier, et 8-bit, 8-bit signed, 16-bit et 16-bit signed l'arrondissent et le limitent à cette largeur, donc la boîte montre ce qui sort vraiment.

## Recette

**Sidechain sous les yeux** Câble la sortie d'un Follower sur l'entrée d'un Number et la sortie du Number sur la prise Gain d'un Gain placé sur la basse. La boîte lit l'enveloppe pendant qu'elle bouge, donc tu vois le niveau qu'atteint le ducking avant de régler la plage du routage.

## Organisms associés

Slider, Sig, Follower, Gain
