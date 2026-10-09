# SideKick

Un façonneur de gain synchronisé au transport, qui fait pomper le signal selon une courbe dessinée, sans entrée de clé.

Une courbe de gain est parcourue une fois par intervalle synchronisé, calée sur la grille du transport pour que la phase 0 tombe sur le temps ; le creux classique plonge exactement là où tombe le kick et reste identique à chaque cycle. Il n'y a ni clé, ni seuil, ni détecteur, seulement la forme. Il ne tourne que pendant la lecture du transport et laisse passer le signal intact à l'arrêt. Son gain et sa phase sont aussi des valeurs de contrôle, donc n'importe quel bouton rotatif peut choisir Contrôler avec SideKick pour pomper en mesure. Mets-le sur une basse, une nappe ou tout un bus de Mixer. Pour un ducking déclenché par un vrai signal audio, utilise SideChain.

## La forme

Clique sur l'une des vignettes sous la courbe pour charger un creux, une montée en volume, un motif de gate, un riser ou un triangle, ou fais glisser sur la courbe pour dessiner à main levée. Les flèches sous l'échelle en dB décalent la forme plus tôt ou plus tard d'un trente-deuxième du cycle, REV la joue à l'envers et INV inverse le gain pour que les creux deviennent des pics.

## Paramètres

**Sync** La longueur du cycle : 1 bar, 1/2 bar, 1/4 bar, 1/8 bar ou 1/16 bar. 1/4 bar correspond à un temps, le pompage classique.

**Mix** À 1, le signal façonné remplace le signal direct ; plus bas, il les mélange pour un pompage en parallèle.

**Smooth** Arrondit les angles vifs de la courbe, en millisecondes. Monte-le si une forme de gate clique, baisse-le pour des hachures plus serrées.

**Shape** La courbe de gain dessinée, stockée avec le patch.

## Recette

**Ducking de club** Sync 1/4 bar, la première vignette de creux, Smooth 3, Mix 1, sur la basse. Fais un clic droit sur le Gain d'une nappe et choisis Contrôler avec SideKick gain pour que la nappe pompe sur la même courbe. Avance la forme d'un cran de flèche pour pomper juste avant le kick.

## Organisms associés

SideChain, Kick, Microdot
