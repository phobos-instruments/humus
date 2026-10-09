# Acid

Un synthé monophonique de lignes, dans la tradition des boîtes acid, avec accent et slide.

Une dent de scie ou une onde carrée à bande limitée passe dans un filtre ladder à diodes résonant, qu'une enveloppe exponentielle ouvre à chaque note. Deux choses font parler ses lignes : une note de vélocité 110 ou plus est un accent, qui ouvre davantage le filtre et frappe plus fort, et deux notes qui se chevauchent glissent en hauteur sans relancer l'enveloppe. Le ladder et les étages de sortie qui façonnent son grave sont notre propre version de ceux d'[Open303](https://github.com/RobinSchmidt/Open303) de Robin Schmidt. Branche un Riff, un Steps ou un PianoRoll sur son entrée MIDI ; quand son éditeur a le focus, le clavier de l'ordinateur en joue directement.

## Paramètres

**Wave** Forme de l'oscillateur : SAW, SQR, PLS ou TRI.

**Cutoff** La fréquence de base du filtre. L'enveloppe balaie vers le haut à partir d'ici.

**Resonance** L'emphase du filtre, d'une atténuation douce jusqu'au squelch à fond.

**EnvMod** Jusqu'où l'enveloppe pousse la fréquence de coupure au-dessus de Cutoff.

**Decay** La vitesse à laquelle le balayage du filtre redescend, en millisecondes.

**Accent** À quel point une note accentuée frappe plus fort. Zéro rend toutes les notes ordinaires.

**Glide** Le temps que met un slide entre deux notes qui se chevauchent pour arriver, en millisecondes. 60 est le réglage classique.

**Attack** La vitesse à laquelle le volume monte au début d'une note, en millisecondes.

**AmpDecay** La vitesse à laquelle une note tenue descend vers Sustain. Tout en haut de sa plage, la note ne descend plus du tout.

**AccentDecay** Le déclin du filtre utilisé sur les notes accentuées, en millisecondes, pour que les accents résonnent plus longtemps ou claquent plus court que les notes ordinaires.

**FilterFM** Renvoie la sortie dans la fréquence de coupure, pour que le filtre se module lui-même. Zéro est propre, plus haut est plus rugueux.

**Track** À quel point la fréquence de coupure suit la hauteur jouée. Zéro garde le filtre fixe, quelle que soit la note.

**Sustain** Le niveau où une note tenue se stabilise après AmpDecay. Zéro, c'est la boîte d'origine, où chaque note s'éteint ; plus haut, la note tient jusqu'à ce qu'on la relâche.

**Squelch** La part de grave que la réinjection du filtre a le droit d'enlever. Bas, la basse tient sous une forte résonance ; haut, elle s'amincit jusqu'au squelch classique.

**Velocity** À quel point la vélocité façonne l'accent. À zéro, un accent est tout ou rien au-dessus de la vélocité 110 ; à un, chaque vélocité fixe sa propre quantité d'accent.

**Punch** L'enveloppe du filtre fait aussi monter le volume, davantage sur les accents. Zéro désactive cet effet.

**Muffler** Un passe-bas sur la sortie, pour adoucir les aigus quand Drive est haut.

**Drive** Saturation en sortie.

**LfoWave** La forme d'une ondulation supplémentaire sur la fréquence de coupure : SIN, TRI, SQR, SAW, DSAW ou S&H.

**LfoRate** La vitesse de l'ondulation en Hz quand LfoSync est désactivé.

**LfoDepth** Jusqu'où l'ondulation déplace la fréquence de coupure. Zéro la coupe.

**LfoSync** Cale l'ondulation sur le transport au lieu de LfoRate.

**LfoBeats** La durée d'un cycle d'ondulation en temps quand LfoSync est activé.

**Level** Volume de sortie.

**BendRange** Jusqu'où va la molette de pitch bend à fond de course, en demi-tons. Deux est la valeur par défaut courante ; zéro ignore la molette.

## Recette

**Ligne squelch** Wave SAW, Cutoff 400, Resonance 8, EnvMod 0.7, Decay 250, Accent 0.6, Glide 60, Drive 0.4. Branche un Riff avec quelques pas accentués et un slide, puis fais bouger Cutoff à la main au fil de la mesure.

## Organisms associés

Riff, Steps, Microdot
