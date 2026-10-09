# RNG

Une source de nombres aléatoires tirés de l'entropie propre de la machine, envoyés comme valeur de contrôle.

Chaque tirage vient de l'entropie du système plutôt que d'une formule, donc la suite ne peut être ni rejouée ni devinée à l'avance. La sortie porte le dernier tirage, qui voyage par un câble de contrôle jusqu'à n'importe quelle prise du patch, ou jusqu'à un Number pour y être lu et transmis. Sur un bouton rotatif, un tirage fait dans une largeur se lit comme une proportion de cette largeur, donc 200 sur 255, c'est presque en haut ; sur un Number, il s'affiche comme le nombre lui-même. L'affichage en bas montre le tirage, et le voyant à côté de Rate clignote à chacun. Les tirages se font sur une horloge, sur le temps du transport, ou à la demande depuis Trigger.

## Paramètres

**Rate** Tirages par seconde quand Sync est désactivé, jusqu'à 50. Zéro arrête l'horloge pour que seul Trigger tire.

**Sync** Prend l'horloge des tirages sur le transport au lieu de Rate, pour que les tirages tombent avec la musique et suivent le tempo. Pendant la lecture du transport, un tirage tombe sur le temps 1 puis à chaque frontière de SyncBeats, où que tu sautes dans la timeline.

**SyncBeats** Le temps musical entre deux tirages quand Sync est activé : un quart de temps pour quelque chose d'agité, quatre pour un changement par mesure.

**Format** Le genre de nombre qu'est un tirage. 0 to 1 est la simple valeur unitaire, Coin est cette valeur arrondie à zéro ou à un, et 8-bit, 8-bit signed, 16-bit et 16-bit signed tirent un nombre entier sur toute cette largeur.

**Trigger** Tire un nombre sur un front montant, pour qu'un Button, une note ou un capteur puisse demander un nombre au lieu d'attendre l'horloge.

## Recette

**Nouvelle fréquence de coupure à chaque mesure** Sync activé, SyncBeats 4, Format 0 to 1. Branche la sortie sur la prise Frequency d'un Filter placé après un organism Drums et règle la plage du routage de 300 à 3000 ; la fréquence de coupure saute à un nouvel endroit au début de chaque mesure.

## Organisms associés

Number, Slider, LFO, Steps
