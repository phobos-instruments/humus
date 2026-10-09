# Filter

Un filtre à variables d'état avec un étage à lampes, dans la tradition du filtre de club en rack.

Un étage de drive alimente un filtre résonant dont les boutons HI, BAND et LO se combinent au lieu de s'exclure : HI avec LO forme un notch à balayer pour un effet de phasing, les trois ensemble forment un passe-tout que Resonance rend spectaculaire, et si aucun n'est sélectionné, le signal passe intact. Un suiveur d'enveloppe et un balayage en triangle ou en carré peuvent tous deux pousser la fréquence de coupure, ou l'enveloppe peut être envoyée dans le drive à la place. Chaque commutation est douce, donc tu peux jouer avec les boutons en live sans clics. Branche-le après un Rhizome, un organism Drums ou un Mixer entier, et relie un LFO ou un Follower à Frequency par un câble pour plus de formes que les balayages intégrés.

## Paramètres

**EnvFollow** À quel point le volume de l'entrée ouvre la fréquence de coupure. Zéro laisse le filtre immobile.

**EnvDecay** Relâchement Fast ou Slow de l'enveloppe une fois que l'entrée retombe.

**Drive** L'étage à lampes. Il ajoute d'abord de la chaleur, puis un écrêtage doux, puis un écrêtage dur à mesure que tu le pousses, avec un gain de compensation pour que le niveau ne bouge pas.

**EnvToDrive** Envoie l'enveloppe dans Drive au lieu de la fréquence de coupure, pour que les passages forts saturent davantage.

**LfoSpeed** Vitesse du balayage, de 0.2 Hz à 10 kHz. Au-delà de quelques centaines de hertz, le balayage lui-même devient un son.

**LfoWave** Triangle glisse entre les extrémités du balayage. Square saute de l'une à l'autre.

**LfoSync** Cale le balayage sur le transport au lieu de LfoSpeed.

**LfoBeats** La durée d'un balayage synchronisé en temps, donc 4 fait une mesure à quatre temps.

**LfoDepth** À quel point le balayage pousse la fréquence de coupure, jusqu'à quelques octaves dans chaque sens.

**Resonance** D'une pente douce à un pic de synthé. Elle se modère d'elle-même aux niveaux d'entrée élevés et aux fréquences de coupure basses, pour que les balayages complets restent sans danger pour les enceintes.

**HiPass** Ajoute la bande passe-haut à la sortie.

**BandPass** Ajoute la bande passe-bande à la sortie.

**LoPass** Ajoute la bande passe-bas à la sortie.

**Frequency** La fréquence de coupure, de 20 Hz à 20 kHz. Le grand balayage.

**FilterOn** Active ou contourne le filtre. Désactivé, c'est un simple fil, propre.

**Mono** Fait passer les filtres des deux canaux en série depuis l'entrée gauche, pour une pente de 24 dB et une résonance doublée, et alimente les deux sorties.

**MixInvert** Sort le signal direct moins le signal filtré, pour des effets d'annulation qui vont bien avant un delay ou une réverb.

## Recette

**Balayage synchronisé** LoPass activé, Resonance 6, Drive 3, Frequency 400. LfoSync activé, LfoBeats 4, LfoWave Triangle, LfoDepth 6. Branche un organism Drums à l'entrée et la fréquence de coupure s'ouvre et se ferme une fois par mesure, en rythme avec le transport.

## Organisms associés

LFO, Gain, Follower
