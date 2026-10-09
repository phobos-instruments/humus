# Sequence

Une grille de batterie à seize pas, d'une à huit rangées, où chaque rangée a sa propre sortie MIDI.

Chaque rangée envoie ses notes par sa propre sortie MIDI, donc la rangée 1 peut piloter un Kick, la rangée 2 un Sampler et la rangée 3 un Silt, et la plaque de nom de la rangée montre à quoi elle est reliée. La dernière sortie est le master et porte toutes les rangées ensemble, pour un seul sampler de batterie qui répartit tout le kit par note. Huit banques contiennent chacune une grille complète ; les notes, vélocités et activations des rangées forment le kit et restent les mêmes d'une banque à l'autre. Le pattern fait une mesure de doubles croches et les modifications s'appliquent pendant que le transport joue.

## La grille

Clique sur un pas pour l'activer, clique encore pour l'effacer, fais glisser pour en peindre une série. La colonne en cours s'allume pendant que le transport joue. La LED à gauche de la rangée l'active, le bouton rotatif règle sa vélocité, et la molette de la souris sur la plaque de nom change la note de la rangée. Fais un clic droit sur un pas pour Aléatoire, Effacer et Copier vers la banque.

## Paramètres

**SwingFollow** Follow fait prendre à la grille le groove du patch depuis le transport, au lieu de son propre bouton Swing.

**Swing** Retarde les doubles croches à contretemps, d'un jeu droit à 0 jusqu'à un shuffle de triolet complet à 1. Utilisé seulement quand Follow est désactivé.

**Gate** Longueur des notes, en fraction d'un pas.

**Enable_1** Si la rangée 1 joue ; les pas sont gardés quand elle est désactivée. Et ainsi de suite pour les rangées 2 à 8.

**Vel_1** Vélocité des notes de la rangée 1. Et ainsi de suite pour les rangées 2 à 8.

**Note_1** La note MIDI que la rangée 1 envoie, par défaut celle de la répartition de batterie générale, 36 pour un kick. Les batteries de synthèse comme Kick l'ignorent ; les samplers s'en servent. Et ainsi de suite pour les rangées 2 à 8.

**Bank** Laquelle des huit grilles joue, de A à H.

**Rows** Combien de rangées a la grille, de 1 à 8. Les sorties suivent : une par rangée, et le master toujours en dernier, donc un câble sur le master reste sur le master quand le nombre change. Les rangées que tu retires gardent leurs pas, leurs notes et leurs vélocités, et reviennent telles quelles si tu les rajoutes ; seuls les câbles de leurs sorties sont retirés.

## Recette

**Four on the floor** Relie la rangée 1 à un Kick, la rangée 2 à un Cicada ou à un clap dans un Sampler, la rangée 3 à un échantillon de charleston. Les pas 1, 5, 9 et 13 sur la rangée 1, 5 et 13 sur la rangée 2, un pas sur deux sur la rangée 3. Swing 0.15 avec Follow désactivé, Gate 0.5. Dessine un fill dans la banque B et change Bank depuis un Button pour la relance.

## Organisms associés

Riff, Steps, PianoRoll
