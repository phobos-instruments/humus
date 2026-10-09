# PinkTrombone

Une voix humaine construite à partir d'un modèle de la gorge, d'après [Pink Trombone](https://dood.al/pinktrombone/) de Neil Thapen.

Pas d'échantillon ni de banque d'oscillateurs : un modèle de la glotte produit la vibration des cordes vocales, et un guide d'ondes qui a la forme d'une gorge, d'une bouche et d'un nez la filtre en voyelles, en fredonnements et en souffles. Joue-le en MIDI : la note tenue donne la hauteur et la vélocité pèse sur le volume. La paire Tongue oriente la voyelle, Squeeze pince le conduit vocal au point Place pour les fricatives et les occlusives, et Nasal ouvre le passage vers le nez. De petites errances de hauteur et de pression gardent une note tenue vivante. Câble un LFO ou un Slider sur TongueX, Squeeze et Nasal et il se met à babiller ; câble sa sortie vers un Harmonizer ou un Trellis.

## Paramètres

**TongueX** Front. Fait glisser la bosse de la langue entre la gorge et les dents ; avec TongueY, il place la voyelle.

**TongueY** Raise. Bombe la langue vers le palais.

**Place** L'endroit où le conduit est pincé : les lèvres en haut du bouton rotatif, la gorge en bas.

**Squeeze** La force du pincement au point Place. Rien pour les voyelles, un peu pour les fricatives, à fond pour bloquer l'air, si bien que le relâcher fait éclater une occlusive.

**Nasal** Ouvre le voile du palais pour que le son s'échappe par le nez. Garde-le ouvert pour fredonner des m et des n, ou colore une voyelle avec un peu de nasal.

**Tenseness** La pression des cordes vocales, du murmure soufflé à la vibration serrée. La vélocité dose le résultat.

**Vibrato** La profondeur de l'ondulation de hauteur voulue et de la lente dérive en dessous.

**Wobble** Exagère l'errance involontaire jusqu'à en faire un lent chevrotement.

**Drone** Fait sonner la voix sans note tenue, à la dernière hauteur reçue.

**Width** Étale la voix sur les sorties stéréo. À 0, les deux côtés portent la même voix ; en montant, les côtés s'écartent comme un chanteur seul dans une pièce, et repliée en mono, la voix reste inchangée.

**Pan** Place la voix à gauche ou à droite. Au centre, les deux côtés passent à plein niveau ; hors du centre, seul le côté opposé est baissé.

**Level** Gain de sortie.

**BendRange** Jusqu'où va la molette de pitch bend à fond de course, en demi-tons. Deux est la valeur par défaut habituelle ; zéro ignore la molette.

## Recette

**Nappe de chœur** Drone activé, Tenseness 0.5, Vibrato 0.3, TongueX 0.3 et TongueY 0.6 pour un « ou ». Envoie une note MIDI pour fixer la hauteur, câble un LFO à 0.1 Hz sur TongueX avec une faible profondeur pour que la voyelle dérive, et fais passer la sortie par un Verbatim.

## Organisms associés

Harmonizer, Trellis, Decomposer
