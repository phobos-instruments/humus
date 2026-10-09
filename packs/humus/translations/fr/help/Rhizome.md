# Rhizome

Un synthé MIDI à six voix qui empile des voix supplémentaires au-dessus de chaque note, espacées en degrés de la gamme de l'accordage du patch.

Chaque note jouée fait pousser une série de stolons : d'autres oscillateurs placés Interval degrés de gamme au-dessus de la fondamentale, dont les hauteurs sont lues dans l'accordage du patch plutôt que dans la série harmonique. Change l'accordage sur le transport et le timbre change avec, pas seulement la hauteur. Les stolons alternent à gauche et à droite pour la largeur, et un passe-bas sur la somme garde sous contrôle une haute pile de dents de scie. Joue-le depuis un PianoRoll, un DNA ou le clavier quand l'éditeur a le focus, et envoie-le dans Fern ou SoundSpace.

## Paramètres

**Runners** Combien de voix poussent de chaque note, de la fondamentale seule jusqu'à huit.

**Interval** L'écart entre deux stolons consécutifs, en degrés de gamme. Affiché comme Degrees sur le panneau.

**Creep** Une lente dérive de hauteur, indépendante pour chaque stolon, en cents. Un peu de dérive empêche la pile de se verrouiller en phase.

**Tilt** La pente de niveau le long de la chaîne. Bas, le son reste proche de la fondamentale ; haut, les stolons lointains ressortent.

**Wave** La forme d'onde des stolons : SIN, SAW ou SQR.

**Bloom** Temps d'attaque en millisecondes, le temps que met une note à s'ouvrir.

**Decay** Temps de relâchement en millisecondes, le temps que met une note à s'éteindre après le note-off.

**Cutoff** Un passe-bas sur la sortie sommée, en Hz. Baisse-le quand beaucoup de stolons en dent de scie deviennent durs.

**Level** Niveau de sortie.

**BendRange** Jusqu'où va la molette de pitch bend à fond de course, en demi-tons. Deux est la valeur par défaut habituelle ; zéro ignore la molette.

## Recette

**Cluster** Interval 1, Runners 6, Creep 0, Wave SIN. Joue des notes seules depuis un PianoRoll : chacune devient une grappe de degrés de gamme voisins qui bat selon l'écart propre à la gamme. Réaccorde le patch sur le transport et le battement change avec.

## Organisms associés

Wave, Mineral, Substrate
