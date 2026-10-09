# VuMeter

Un vumètre à deux aiguilles, dans la tradition du pont de vumètres d'un magnétophone à bande. Il ne fait qu'écouter : il n'a pas de sortie, donc branche-y un second câble depuis ce que tu veux mesurer, et le retirer ne coupe jamais le son.

Les aiguilles bougent lentement avec un léger dépassement, mais l'échelle est en décibels sous le plafond numérique : 0 en haut de l'arc, la zone rouge à partir de -6. Un signal qui évolue dans le tiers supérieur et respire vers le rouge, c'est sain ; une aiguille qui reste collée en haut veut dire qu'il faut baisser. Un voyant à côté de chaque cadran s'allume quand un seul échantillon atteint -1 dBFS et reste allumé un instant, pour qu'un transitoire isolé ne puisse pas se cacher entre deux mouvements de l'aiguille. Un câble dans la seule entrée 1 s'affiche sur les deux cadrans.

## Recette

**Réglage des gains** Branche aussi dedans ce qui alimente le SoundOut. Règle chaque Gain et chaque canal du Mixer pour que les aiguilles évoluent dans le tiers supérieur sur le passage le plus fort et que les voyants ne clignotent que sur les plus gros coups. Si un voyant reste allumé, baisse le master du Mixer plutôt que le SoundOut.

## Organisms associés

SoundOut, Console, Send
