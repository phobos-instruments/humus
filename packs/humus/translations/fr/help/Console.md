# Console

Une console de sommation analogique : un mixeur dont les canaux partagent un même rail d'alimentation et un même transformateur de sortie, si bien qu'ils réagissent les uns aux autres.

Là où un Mixer additionne ses entrées, une Console modélise ce qui fait la cohésion d'une console. Chaque canal est une paire stéréo numérotée d'après ses entrées, 1-2, 3-4 et ainsi de suite. Gain et Pan placent chaque canal sur le bus, Crosstalk fait fuir le bus sur lui-même, Sag fait baisser tout le bus selon l'effort qu'il fournit, et Drive sature le transformateur. Comme Sag et Drive lisent le bus additionné, une grosse caisse forte sur un canal fait baisser et déforme tous les autres canaux ; c'est ça, la cohésion, et c'est pour ça qu'il faut écouter la console avec le mix qui joue plutôt que sur un seul coup. C'est un bus, pas une tranche de console : il n'y a ni EQ ni compresseur, donc mets un TransientShaper ou un SideChain sur chaque source avant elle. Branche-la là où irait un Mixer, avant le SoundOut.

## La console

Le menu déroulant Inputs règle combien de canaux stéréo la console additionne, de 2 à 8, et change la classe de l'organism sur place, pour que son nom, ses réglages, ses câbles et son automation survivent. Agrandir ajoute des canaux en bas ; réduire retire les plus hauts et leurs câbles. Un câble dans l'entrée gauche d'un canal alimente les deux côtés, donc une source mono se place au centre et Pan la positionne.

## Paramètres

**Output** Réglage de niveau général, appliqué en dernier, après toute la coloration.

**Direct** Désactive tous les étages de coloration mais garde la sommation et Output. Activé, le bus est la simple somme de ses canaux, pour un A/B honnête.

**Drive** La force avec laquelle le transformateur est poussé. Sa saturation est asymétrique : elle épaissit le bas du spectre et arrondit les transitoires ; à zéro, l'étage est hors circuit.

**Crosstalk** La quantité d'aigus de chaque côté qui déborde sur le côté opposé. Subtil par conception, avec un maximum vers -30 dB ; il s'entend surtout sur une matière qui diffère entre la gauche et la droite.

**Sag** Jusqu'où la demande additionnée fait baisser le rail, qui remonte quand la musique respire. Bas et lent, c'est de la cohésion ; plus haut, ça pompe avec l'élément le plus fort de la console.

**Flavor** Donne une couleur aux trois couplages à la fois. Clean est une sommation transparente. British est chaud et soudé, avec un rail lent. American est percutant, avec un rail plus rapide et un grain plus dur. Modern est presque linéaire : un rail rigide, peu de diaphonie, presque pas de fer.

**Gain_1-2** Niveau du canal 1 dans le bus. Et ainsi de suite pour 3-4 à 11-12.

**Pan_1-2** Balance du canal 1. Au centre, les deux côtés passent à plein niveau ; s'écarter du centre ne fait que baisser le côté opposé. Et ainsi de suite pour 3-4 à 11-12.

**Mute_1-2** Rend le canal 1 muet, ce qui soulage aussi le rail pour les autres. Et ainsi de suite pour 3-4 à 11-12.

**Solo_1-2** Rend muets tous les canaux sauf celui-ci. Le Mute propre à un canal l'emporte toujours. Et ainsi de suite pour 3-4 à 11-12.

**Name_1-2** Le ruban au-dessus du canal 1. Vide tant que rien n'est branché, puis il montre le nom de l'organism branché. Double-clique pour écrire le tien ; efface-le pour revenir au nom de ce qui est branché. Et ainsi de suite pour 3-4 à 11-12.

## Recette

**Souder un mix** Choisis Flavor en premier, car il déplace la cible de tout le reste. Règle les Gain des canaux pour l'équilibre, avec Drive et Sag bas, monte Sag jusqu'à ce que le mix respire avec la grosse caisse, puis ajoute Drive en dernier pour du poids. Si ça pompe de façon trop évidente, baisse le canal le plus fort plutôt que Sag, et récupère le niveau perdu avec Output.

## Organisms associés

Mixer, Bus, SideChain, TransientShaper
