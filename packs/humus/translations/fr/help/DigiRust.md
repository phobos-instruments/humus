# DigiRust

Un canal lo-fi qui imite un convertisseur bon marché en train de lâcher : moins de bits, une horloge plus lente, une horloge qui vacille et du souffle.

Bits réduit le signal à moins de niveaux et Rate tient chaque échantillon plus longtemps, pour que les aigus se replient vers le bas en grain. Jitter fait vaciller l'horloge, donc la durée de maintien varie d'un échantillon à l'autre, et Noise ajoute un souffle qui suit le signal et s'efface dans le silence, si bien qu'un patch au repos reste silencieux. Tone incline le résultat vers un son plus sombre ou plus présent, Mix le mélange avec l'entrée directe pour un grain en parallèle, et un bloqueur de composante continue garde la sortie centrée. Aléatoire tire au hasard Bits, Rate, Jitter, Noise et Tone mais laisse Mix et Level tranquilles : un tirage change la couleur, jamais l'équilibre du patch. Branche-le après un Sampler ou un organism Drums, ou avant un Fern pour un écho dégradé.

## Paramètres

**Bits** Longueur de mot, de 1 à 16, par pas entiers. Plus bas, c'est plus grossier.

**Rate** Horloge d'échantillonnage en Hz. En haut de sa course, il sort du trajet du signal ; bas, ça craque.

**Jitter** Vacillement de l'horloge. Zéro, c'est une horloge stable ; en montant, ça ondule et ça bave.

**Noise** Souffle asservi au signal. Zéro, c'est le silence.

**Tone** Inclinaison d'un son sombre à un son présent, autour du milieu.

**Mix** Mélange du direct au rouillé.

**Level** Niveau de sortie.

## Recette

**Batterie de vieux sampler** Bits 10, Rate 22000, Jitter 0.1, Noise 0.15, Tone 0.4, Mix 1. Branche un Sampler qui joue un break et les aigus s'émoussent dans le craquement classique des premiers samplers ; descends Rate à 8000 pour le breakdown.

## Organisms associés

Sampler, Fern, Filter
