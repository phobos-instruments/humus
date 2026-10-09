# Paulstretch

Un étirement temporel extrême qui transforme un fichier son en un nuage lent, sans phase.

Il suit l'algorithme [Paulstretch](https://github.com/paulnasca/paulstretch_python) de Nasca Octavian Paul, qui est dans le domaine public. Ce n'est pas un outil d'étirement pour le travail de tempo : en dessous d'environ quatre fois, il étale, et cet étalement, c'est l'instrument. La tête de lecture avance dans le fichier à une vitesse divisée par Stretch ; chaque pas est fenêtré, son spectre gardé et ses phases tirées au hasard à chaque fois, si bien que l'harmonie reste et que l'instant s'en va. Les deux canaux tirent leurs phases indépendamment, donc un fichier mono ressort large. Donne-lui un accord, une voix ou un morceau entier et branche la sortie sur un Fern ou un Verbatim pour une nappe de fond, ou directement sur un Mixer. La bande de transport sous le fichier permet de se déplacer dans la source, et chaque bouton de cette bande est un paramètre : fais un clic droit sur l'un d'eux pour choisir Apprentissage MIDI, Apprentissage OSC, Contrôler avec ou Suivre. Le Stop du transport le laisse jouer ; appuie une seconde fois sur Stop, quand plus rien ne tourne, et il s'arrête avec tout le reste.

## Paramètres

**File** Le fichier son source.

**Stretch** Combien de fois plus lent que le temps réel, de 1 à 100. Huit, c'est le réglage classique ; à cinquante, ça bouge à peine.

**Window** La fenêtre d'analyse en secondes. Courte, elle garde un peu de rythme et de gargouillis ; longue, elle dissout tout en nappe.

**Loop** Fait boucler la source pour que le nuage ne finisse jamais. Désactivé, il la lit une fois puis retombe dans le silence.

**Active** Active la lecture. Désactivé, c'est le silence.

**Level** Gain de sortie.

## Recette

**Nappe à partir d'un accord** Charge un accord de deux secondes, Stretch 30, Window 0.5, Loop activé. Branche la sortie sur un Verbatim avec une longue queue et baisse Level jusqu'à ce que la nappe se glisse sous le reste du patch.

## Organisms associés

SpectralFreeze, FilePlayer, Bloom
