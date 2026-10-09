# Substrate

Un drone ou une nappe évolutive, fait d'au plus 64 couches de dents de scie désaccordées, empilées sur une échelle d'intervalles.

Chaque couche monte d'un barreau d'Interval au-dessus de la fondamentale et porte sa propre lente dérive de hauteur et de niveau ; les couches alternent dans le champ stéréo. Interval est demandé à l'accordage du patch : sous un organism Tuning, l'échelle parcourt les degrés propres à cette gamme. L'échelle repart du bas après huit barreaux : avec beaucoup de couches, elles s'empilent en copies à l'unisson qui dérivent, et le niveau reste stable quand on en ajoute. Les presets contiennent les échelles Octaves, Fifths et Thirds. Joue-le depuis un PianoRoll ou un DNA, ou laisse Drone activé et pilote Cutoff depuis un Metapad.

## Paramètres

**Drone** Désactivé, il joue en MIDI à travers Attack et Release. Activé, il sonne en continu sur Note, toujours comme une seule fondamentale, quoi que dise Mode.

**Note** La fondamentale quand Drone est activé, ou tant que rien n'a été joué en mode Mono.

**Layers** Combien de strates, de 1 à 64. Au-delà d'environ 16, ce n'est plus un accord de couches mais une texture.

**Interval** De combien de degrés de gamme monte chaque barreau. En accordage ordinaire, 12 empile des octaves, 7 des quintes et 4 des tierces ; toute valeur intermédiaire donne une nappe qu'aucune échelle fixe ne propose.

**Spread** La profondeur de dérive en cents, jusqu'où les strates s'écartent.

**Motion** Combien la dérive fait bouger la hauteur et le niveau. 0 fige la nappe.

**Cutoff** Le passe-bas de la nappe. Automatise-le pour faire gonfler la nappe.

**Attack** Le temps que met la nappe à s'ouvrir, en millisecondes.

**Release** Le temps qu'elle met à s'éteindre une fois les notes relâchées.

**Level** Niveau de sortie.

**BendRange** Jusqu'où va la molette de pitch bend à fond de course, en demi-tons. Deux est la valeur par défaut habituelle ; zéro ignore la molette.

**Mode** Comment les notes MIDI deviennent des voix. Mono, c'est une seule fondamentale, la dernière note l'emporte. Para répartit la réserve de couches entre toutes les notes tenues : un accord de trois sons avec 12 couches donne à chaque note une pile de quatre, pour le même coût quel que soit le nombre de touches enfoncées. Poly donne à chaque note la pile complète avec sa propre enveloppe, sur huit voix.

## Recette

**Nappe redistribuée** Drone désactivé, Mode Para, Layers 24, Interval 7, Motion 0.6, Attack 1500, Release 3000, Cutoff 1200. Joue des accords lents et tenus depuis un PianoRoll ; les couches se répartissent entre les notes tenues et reviennent vers celles qui restent tenues quand une note est relâchée.

## Organisms associés

Tuning, Rhizome, DNA
