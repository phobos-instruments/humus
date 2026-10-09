# Silt

Un synthé à puce à trois voix qui émule le MOS SID, la puce sonore de l'ordinateur domestique, grâce au cœur d'émulation [reSID](https://github.com/daglem/reSID).

Le 6581 d'origine et sa révision 8580, plus propre, sont tous deux émulés cycle par cycle, filtre analogique compris. Les trois voix partagent un seul filtre, comme sur le matériel ; une quatrième note vole la voix la plus ancienne. Chaque voix joue la même onde, la même largeur d'impulsion et l'enveloppe à quatre étapes propre à la puce, dans ses seize pas natifs. La hauteur vient de l'accordage du patch, à la résolution des registres de fréquence de la puce, donc une gamme microtonale se joue. Le mode Twin ajoute une deuxième puce, comme sur les cartes modifiées, et double chaque note avec la paire désaccordée à travers l'image stéréo. Joue-le par un câble MIDI depuis un PianoRoll, un DNA ou un MidiIn, ou au clavier quand son éditeur a le focus.

## Paramètres

**Model** Quelle puce : 6581, avec son filtre chaud et capricieux, ou 8580, avec le filtre corrigé. Le changement fait remonter brièvement le niveau en fondu, pour adoucir le saut entre les tensions de repos des deux puces.

**Wave** TRI, SAW, PLS ou NSE, les quatre formes d'onde de la puce.

**PulseWidth** Le rapport cyclique de l'onde d'impulsion. Audible seulement sur PLS.

**Attack** L'attaque de l'enveloppe, dans les seize pas de la puce. 0 est instantané.

**Decay** Le déclin de l'enveloppe jusqu'au niveau de maintien, en seize pas.

**Sustain** Le niveau tenu tant que la note est enfoncée, en seize pas.

**Release** Le relâchement de l'enveloppe après le note-off, en seize pas. 15 résonne pendant des secondes.

**Cutoff** L'endroit où le filtre se ferme, sur toute la plage des registres du matériel.

**Resonance** L'intensité avec laquelle le filtre chante à la fréquence de coupure, en seize pas.

**FilterMode** LOW, BAND ou HIGH : passe-bas, passe-bande ou passe-haut.

**Filter** Retire complètement le filtre pour des voix brutes, plus brillantes et plus fortes.

**Ring** Chaque voix est modulée en anneau par sa voisine, d'où un son métallique et cliquetant. L'effet s'entend sur l'onde TRI.

**Sync** Chaque voix se cale en hard sync sur sa voisine.

**Chips** ONE pour une seule puce, ou TWIN : chaque note jouée sur les deux puces, chacune en tête d'un côté avec un peu de l'autre mélangée dedans.

**TwinDetune** L'écart de désaccord de la paire twin, en cents, de part et d'autre de la note. Affiché comme Spread.

**Chain** Fait passer la sortie de la première puce par le filtre de la deuxième : le côté gauche arrive brut et le côté droit à travers un second filtre analogique. TWIN seulement.

**ModelB** Le modèle de la deuxième puce, B 6581 ou B 8580. Mélanger un original chaud avec la révision propre, c'est la moitié de l'intérêt d'en avoir deux.

**Level** Niveau de sortie.

**BendRange** Jusqu'où va la molette de pitch bend en fin de course, en demi-tons. Deux est la valeur par défaut courante ; zéro ignore la molette.

## Recette

**Basse chip** Model 6581, Wave PLS, PulseWidth 0.25, Attack 0, Decay 9, Sustain 6, Release 5, FilterMode LOW, Cutoff 0.45, Resonance 8. Câble un Riff sur l'entrée MIDI et ajoute un LFO sur PulseWidth à un quart de mesure pour le balayage d'impulsion classique. Passe en TWIN avec TwinDetune 8 pour l'élargir.

## Organisms associés

pH, Acid, Rhizome, PianoRoll
