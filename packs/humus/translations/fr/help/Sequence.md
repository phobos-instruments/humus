# Sequence

Une grille de batterie à seize pas, d'une à huit rangées, où chaque rangée a sa propre sortie MIDI.

Chaque rangée envoie ses notes par sa propre sortie MIDI, donc la rangée 1 peut piloter un Kick, la rangée 2 un Sampler et la rangée 3 un Silt, et la plaque de nom de la rangée montre à quoi elle est reliée. La dernière sortie est le master et porte toutes les rangées ensemble, pour un seul sampler de batterie qui répartit tout le kit par note. Huit banques contiennent chacune une grille complète ; les notes, vélocités et activations des rangées forment le kit et restent les mêmes d'une banque à l'autre. Le pattern fait une mesure de doubles croches et les modifications s'appliquent pendant que le transport joue.

## La grille

Clique sur un pas pour l'activer, clique encore pour l'effacer, fais glisser pour en peindre une série. La colonne en cours s'allume pendant que le transport joue. La LED à gauche de la rangée l'active, le bouton rotatif règle sa vélocité, et la molette de la souris sur la plaque de nom change la note de la rangée. Fais un clic droit sur un pas pour Aléatoire, Effacer et Copier vers la banque. Fais un clic droit sur un pas, la LED ou la plaque de nom pour Exclure la rangée de l'aléatoire : chaque tirage aléatoire, celui de la grille compris, laisse alors les pas et la vélocité de cette rangée tels quels, et un cadenas apparaît sur sa plaque. Pour ne garder qu'une vélocité, fais un clic droit sur son bouton rotatif et choisis Exclure de l'aléatoire, comme sur n'importe quel bouton.

**Probabilité du pas** Clique sur l'interrupteur % sous Gate pour voir la probabilité de chaque pas allumé sous forme de barre, puis fais glisser vers le haut ou le bas sur un pas pour régler la fréquence à laquelle il joue, de jamais à toujours, par pas de 5 %. Cmd-glisser un pas (Ctrl-glisser sous Windows et Linux) fait de même sans changer de mode. L'interrupteur % est le paramètre View, donc un bouton ou un pad assigné le bascule aussi. Le bouton à côté de la vélocité de chaque rangée est la Chance de la rangée : elle vaut pour tous les pas de cette rangée, et un seul fader éclaircit une ligne de charleston. Fais un clic droit sur le bouton, ou sur un pas de la rangée dans la vue Chance, pour l'assigner. Un pas sous 100 % apparaît plus pâle et affiche sa probabilité sur lui. Les dés appartiennent à la boîte : un patch joue les mêmes pas à chaque lecture et à chaque export, et deux boîtes lancent chacune les leurs. Chaque banque garde ses probabilités, Copier vers la banque les emporte et Effacer les remet à coup sûr.

## Paramètres

**SwingFollow** Follow fait prendre à la grille le groove du patch depuis le transport, au lieu de son propre bouton Swing.

**Swing** Retarde les doubles croches à contretemps, d'un jeu droit à 0 jusqu'à un shuffle de triolet complet à 1. Utilisé seulement quand Follow est désactivé.

**Gate** Longueur des notes, en fraction d'un pas.

**Enable_1** Si la rangée 1 joue ; les pas sont gardés quand elle est désactivée. Et ainsi de suite pour les rangées 2 à 8.

**Vel_1** Vélocité des notes de la rangée 1. Et ainsi de suite pour les rangées 2 à 8.

**Note_1** La note MIDI que la rangée 1 envoie, par défaut celle de la répartition de batterie générale, 36 pour un kick. Les batteries de synthèse comme Kick l'ignorent ; les samplers s'en servent. Et ainsi de suite pour les rangées 2 à 8.

**Bank** Laquelle des huit grilles joue, de A à H.

**View** Steps ou Chance : ce que la grille montre et modifie. L'interrupteur % sous Gate le change.

**Chance_1 à Chance_8** La fréquence à laquelle jouent les pas de chaque rangée, en plus de la probabilité de chaque pas.

**Rows** Combien de rangées a la grille, de 1 à 8. Les sorties suivent : une par rangée, et le master toujours en dernier, donc un câble sur le master reste sur le master quand le nombre change. Les rangées que tu retires gardent leurs pas, leurs notes et leurs vélocités, et reviennent telles quelles si tu les rajoutes ; seuls les câbles de leurs sorties sont retirés.

## Recette

**Four on the floor** Relie la rangée 1 à un Kick, la rangée 2 à un Cicada ou à un clap dans un Sampler, la rangée 3 à un échantillon de charleston. Les pas 1, 5, 9 et 13 sur la rangée 1, 5 et 13 sur la rangée 2, un pas sur deux sur la rangée 3. Swing 0.15 avec Follow désactivé, Gate 0.5. Dessine un fill dans la banque B et change Bank depuis un Button pour la relance.

## Organisms associés

Riff, Steps, PianoRoll
