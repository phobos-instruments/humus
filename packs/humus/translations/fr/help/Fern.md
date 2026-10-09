# Fern

Un écho stéréo synchronisable au tempo, dont les répétitions s'amincissent, s'assombrissent et se réchauffent en s'éteignant.

Body, Damp et Drive se trouvent dans la boucle de réinjection, donc chaque passage dans la boucle les applique à nouveau : la première répétition est façonnée une fois, la cinquième cinq fois. C'est ce qui donne l'impression qu'une queue s'éloigne au lieu d'être copiée. Au-delà des trois quarts de Feedback, il tient une note aussi longtemps que tu veux, et le point de lecture bouge à chaque échantillon, donc tourner Time à la main fait plier la hauteur au lieu de cliquer.

## Paramètres

**Mode** Stereo garde chaque côté pour lui. Ping-pong croise la réinjection pour que chaque répétition tombe du côté opposé à la précédente.

**Time** Temps de delay en millisecondes. Ignoré quand Sync est activé.

**Sync** Cale le temps sur le transport au lieu des millisecondes.

**SyncMultiplier** Combien d'unités SyncUnit font une répétition quand Sync est activé.

**SyncUnit** La valeur de note qui sert d'unité au temps synchronisé, comme 1/8 ou 1/16.

**Feedback** La part de chaque répétition qui devient la suivante. Bas, c'est un slapback ; haut, c'est une queue qui survit à la note.

**Spread** Décale le temps du côté droit par rapport au gauche, ce qui ouvre l'écho dans l'image. Zéro garde les deux côtés ensemble.

**Body** Amincit chaque répétition par le bas, pour qu'une queue s'écarte du chemin de la basse au lieu de s'empiler dessous.

**Damp** Assombrit chaque répétition par le haut. C'est le réglage qui transforme un écho numérique propre en pièce ou en bande.

**Drive** Réchauffe chaque répétition. À zéro, la boucle est propre ; plus haut, la queue s'épaissit en s'éteignant.

**Glide** Ce qui se passe quand Time change. Tape fait glisser le point de lecture, donc les répétitions plient en hauteur au passage. Fade fait un fondu enchaîné des anciennes répétitions vers une nouvelle série au nouveau temps, sans changement de hauteur ; utilise-le quand Time suit le transport.

**Mix** La quantité d'écho entendue face au signal direct.

## Recette

**Queue dub** Sync activé, SyncUnit 1/8 avec un multiplicateur de 3 pour une sensation pointée, Feedback autour de 0.7, Damp 0.6, Drive 0.3, Mode Ping-pong. Envoie un stab court et laisse la queue porter le rythme ; monte Feedback vers le haut pour une montée, puis redescends-le pour dégager.

## Organisms associés

Repeater, SoundSpace, Trellis
