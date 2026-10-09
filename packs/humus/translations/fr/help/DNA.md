# DNA

Un séquenceur génératif qui joue un génome de seize bases sous forme de notes MIDI.

Seize bases, A, C, G et T, tournent en boucle au rythme de Rate, calées sur le transport, et chaque base fait sonner son intervalle au-dessus de Root : celui du voicing de Chord, ou un intervalle à toi, réglé sur les touches de base en bas de l'éditeur. À chaque mesure, Mutate donne une chance qu'une base change, si bien que la ligne évolue lentement pendant qu'elle joue, tandis que la même Seed fait toujours pousser le même brin de départ. La bande au-dessus des lignes de bases montre le pas en cours et les silences du passage actuel ; clique sur une case pour faire taire ce pas pour de bon, fais glisser pour en balayer plusieurs, clic droit pour tout le brin. Relie la sortie MIDI à un Acid, un Mineral ou un Substrate.

## Paramètres

**Root** La tonalité, sous forme de note MIDI.

**Chord** Le voicing que prennent les quatre bases : Minor, Major, Phrygian, Pentatonic, Dorian, Harmonic Minor, Whole Tone, Hirajoshi ou Harmonic (JI), la série 4:5:6:7 en intonation juste.

**Rate** La longueur d'un pas, de 1/64 à 1/1, avec des valeurs en triolet et pointées entre les deux.

**Gate** La durée des notes, en fraction du pas.

**Rest** La part du brin clairsemée en silences. Les silences tombent selon Seed, et monter Rest ne fait qu'ajouter des silences à la même ligne.

**Mute** Les pas rendus muets à la main sur la bande, un bit par pas. Un pas muet ne sonne jamais, quoi que fassent Rest ou Mutate.

**Octaves** Avec 2 ou 3, certains pas sautent d'une octave vers le haut pour élargir la tessiture. 1 laisse chaque base à sa place.

**Mutate** La chance par mesure qu'une base change. Zéro donne un riff fixe ; une valeur haute le rend agité.

**Velocity** La vélocité de chaque note.

**Seed** Quel brin pousse. La même Seed donne toujours le même départ.

**Base A** L'intervalle que joue la base A au-dessus de Root, de 0 à 12 demi-tons. -1 suit le voicing de Chord.

**Base C** Pareil pour la base C.

**Base G** Pareil pour la base G.

**Base T** Pareil pour la base T.

**SwingFollow** Follow fait prendre au motif le groove du patch depuis le transport, au lieu du bouton rotatif Swing.

**Swing** Retarde un pas sur deux vers un ressenti de triolet. Utilisé seulement quand SwingFollow est désactivé.

## Recette

**Acid qui évolue** Relie la sortie MIDI à un Acid. Root 45, Chord Phrygian, Rate 1/16, Gate 0.5, Rest 0.3, Octaves 2, Mutate 0.2. Lance la lecture et laisse tourner quelques mesures ; quand un brin qui te plaît apparaît, ramène Mutate à zéro pour le garder.

## Organisms associés

Mineral, Substrate, Rhizome
