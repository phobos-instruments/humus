# Invert

Inverse la polarité d'un signal : chaque échantillon multiplié par moins un.

Utilise-le quand deux copies d'un signal se rejoignent et que la somme sonne maigre, comme avec un couple de micros ou une chaîne d'effets en parallèle ; inverser une des branches rend souvent du corps. Il crée aussi une image pseudo-stéréo large à partir d'une source mono quand la copie normale va d'un côté d'un Mixer et la copie inversée de l'autre, et il soustrait un signal d'un autre quand les deux arrivent sur le même Bus. Il prend autant de canaux qu'on lui en relie, et l'éditeur de propriétés affiche un scope de champ stéréo de ce qui en sort, pour qu'une annulation se voie autant qu'elle s'entend. Il n'a aucun paramètre.

## Recette

**Test d'annulation** Relie une source à une tranche du Mixer directement, et une seconde fois à une autre tranche en passant par un effet et un Invert. Effet en bypass, le Mixer devient silencieux ; active l'effet et ce qui reste est exactement ce que l'effet ajoute.

## Organisms associés

Gain, Mixer
