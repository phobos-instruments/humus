# pH

Un synthétiseur FM à quatre moteurs : un moteur à six opérateurs qui lit les banques DX7 .syx, et des émulations des puces OPN2, OPM et OPL3.

C'est la banque qui choisit le moteur. Un .syx contient des voix à six opérateurs ; un .wopn, .tfi ou .dmp contient des instruments OPN2 pour le cœur [Nuked-OPN2](https://github.com/nukeykt/Nuked-OPN2) ; un .opm contient des voix OPM pour [Nuked-OPM](https://github.com/nukeykt/Nuked-OPM) ; un .wopl contient des instruments OPL3 pour [Nuked-OPL3](https://github.com/nukeykt/Nuked-OPL3). Charges-en une et le bon moteur la joue : huit voix sur le moteur à six opérateurs, six canaux sur l'OPN2, huit sur l'OPM et dix-huit sur l'OPL3. La hauteur vient du Tuning du patch, donc une gamme microtonale fonctionne aussi sur les voix des puces. Un patch nomme sa banque au lieu de la contenir, alors partage la banque avec le patch. Relie-y un PianoRoll, un DNA ou un MidiIn par un câble.

## Paramètres

**Patch** Quelle voix de la banque joue, par son nom. Choisir une banque repart de sa première voix.

**Bright** Avec quelle force les modulateurs attaquent leurs porteuses. Au centre, c'est la voix telle qu'elle est écrite ; vers le haut, ça ajoute des harmoniques, vers le bas, le son se referme vers une sinusoïde.

**Attack** Le temps que met chaque opérateur à s'ouvrir, autour de ce que la voix demande.

**Release** Le temps que met la voix à s'éteindre après le relâchement de la touche.

**Detune** Écart entre les opérateurs, en cents. De petites valeurs épaississent une voix ; de grandes la font battre contre elle-même.

**Vibrato** Profondeur de l'ondulation de hauteur basse fréquence propre à la voix.

**Speed** La vitesse de cette ondulation.

**Transpose** Décale la note en demi-tons. Il déplace la fréquence entendue plutôt que le numéro de note, donc ça tient dans n'importe quel accordage.

**Fine** Décale la note en cents, de la même façon.

**Level** Niveau de sortie.

**BendRange** Jusqu'où va la molette de pitch bend à fond de course, en demi-tons. Deux est la valeur par défaut courante ; zéro ignore la molette.

**Algorithm** Quel opérateur module lequel, et lesquels arrivent à la sortie. Le moteur à six opérateurs en propose 32 ; les puces en proposent 8.

**Feedback** Avec quelle force le premier opérateur se replie sur lui-même, de 0 à 7.

**Op1_Level** Niveau de sortie de l'opérateur 1, et ainsi de suite pour les opérateurs 2 à 6. Choisir un patch charge sa voix dans ces rangées, donc ce qui est à l'écran est ce qui joue.

**Op1_Ratio** Rapport de fréquence de l'opérateur 1 par rapport à la note, et ainsi de suite pour 2 à 6.

**Op1_Detune** Décalage fin de l'opérateur 1, centré sur 7, et ainsi de suite pour 2 à 6.

**Op1_Attack** Vitesse d'attaque de l'enveloppe de l'opérateur 1, et ainsi de suite pour 2 à 6.

**Op1_Decay** Vitesse de déclin de l'enveloppe de l'opérateur 1, et ainsi de suite pour 2 à 6.

**Op1_Sustain** Niveau de maintien de l'enveloppe de l'opérateur 1, et ainsi de suite pour 2 à 6.

**Op1_Release** Vitesse de relâchement de l'enveloppe de l'opérateur 1, et ainsi de suite pour 2 à 6.

**Op1_On** Retire l'opérateur 1 de la voix ou l'y remet, en gardant son niveau entre-temps, et ainsi de suite pour 2 à 6. Les puces ont quatre opérateurs, donc Op5_On et Op6_On sont indisponibles tant qu'une banque de puce est chargée.

**File** La banque dans laquelle jouer, et avec elle le moteur. Vide, c'est la banque d'usine ; une banque déposée dans le dossier des banques rejoint la liste et masque une banque livrée du même nom.

## Recette

**Piano électrique de puce** Charge la banque de puce d'usine et choisis un patch de piano électrique. Bright 0.6, Detune 6, Release 0.6, Vibrato 0.1, Speed 0.4. Relie-y un PianoRoll par un câble, et un Fern après avec Feedback autour de 0.4 pour l'écho ; désactive puis réactive Op4_On pendant que ça joue pour entendre ce qu'apporte l'opérateur du haut.

## Organisms associés

Rhizome, Wave, PianoRoll, DNA
