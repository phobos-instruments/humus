# Leafcutter

Un découpeur de boucles qui coupe un fichier son sur ses transitoires et joue les tranches en rythme avec le transport, à leur hauteur d'origine.

Charge une boucle et elle est coupée sur ses transitoires ; les tranches suivent ensuite le tempo du transport sans étirement temporel, donc un break reste propre à n'importe quel tempo. Les tempos plus rapides tronquent chaque tranche, les plus lents laissent de l'air entre les coupes. Les fichiers de boucles découpées, .rx2 et l'ancien .rex, se chargent avec leur propre carte des tranches, leur tempo et leur nombre de temps. L'entrée MIDI joue les tranches comme un kit, C3 pour la première tranche et chaque touche au-dessus pour la suivante, pour qu'un PianoRoll ou un clavier puisse jouer la boucle avec ou sans la boucle automatique en dessous. Relie la sortie par un câble à un Filter ou à un Fern.

## La carte

La carte affiche la boucle avec un repère à chaque coupe et allume la tranche en cours de lecture. Fais glisser vers le haut ou vers le bas sur une tranche pour la réaccorder seule, et fais un clic droit dessus pour l'inverser, la rendre muette ou la réinitialiser, ou pour effacer toutes les modifications des tranches. Ces modifications vivent dans SliceEdits et restent avec le patch.

## Paramètres

**File** La boucle à découper. N'importe quel fichier son, ou un fichier de boucle découpée avec sa propre carte des tranches.

**Play** La boucle qui joue toute seule. Désactivé, il ne reste que le kit MIDI.

**Sense** Combien de transitoires détectés deviennent des coupes. La détection a lieu une fois au chargement, donc le bouton rotatif redécoupe instantanément.

**Gate** Quelle part de la fenêtre de chaque tranche sonne, de haché à entier.

**Pitch** Transpose toutes les tranches en demi-tons, de -24 à 24, par rééchantillonnage.

**Reverse** Joue toutes les tranches à l'envers.

**Decay** Fait disparaître chaque tranche en fondu depuis son début. 0 laisse les tranches entières ; tout en haut, chaque tranche est un coup bref.

**Shuffle** Graine de l'ordre des tranches. 0 joue la boucle telle qu'elle est écrite ; tout autre nombre donne un réarrangement reproductible.

**Beats** Longueur de la boucle en temps. 0 la prend dans le fichier, ou la devine à partir de la durée et du tempo.

**Level** Niveau de sortie.

**SliceEdits** Les modifications de hauteur, d'inversion et de mise en sourdine faites tranche par tranche sur la carte, enregistrées avec le patch.

## Recette

**Break redécoupé** Charge une boucle de batterie de deux mesures, Beats 8, Sense 0.6, Gate 0.7, Shuffle 17, Decay 0.2. Lance la lecture et le break joue réarrangé en rythme ; appuie sur Aléatoire pour un autre ordre et relie la sortie par un câble à un Filter avec LfoSync activé.

## Organisms associés

Sampler, Deck, PianoRoll, Repeater
