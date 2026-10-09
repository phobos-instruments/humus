# SideChain

Un compresseur dont la réduction de gain est pilotée par une entrée Key séparée.

Le signal Main sur les entrées 1-2 est baissé selon le niveau du signal Key sur les entrées 3-4 : câble la grosse caisse sur Key et la basse ou la nappe sur Main, et le signal principal s'efface en rythme avec la grosse caisse. Le signal Key ne sert qu'à la détection et ne ressort jamais, alors câble aussi la grosse caisse seule vers ton Mixer. La liste Mode bascule entre stéréo et mono sur place ; en mono, l'entrée 1 est Main et l'entrée 2 est Key. Sans rien sur Key, le signal principal passe intact. Pour un ducking identique à chaque coup, quelle que soit la grosse caisse, utilise plutôt SideKick.

## Paramètres

**Detector** RMS fait la moyenne du signal Key : la réduction accroche tard et relâche lentement, c'est le pompage classique. Peak suit exactement chaque transitoire.

**Threshold** Le niveau du signal Key au-dessus duquel la réduction commence, en dB. Règle-le pour que la grosse caisse le dépasse nettement.

**Ratio** La force avec laquelle le signal principal est baissé une fois que le signal Key dépasse le seuil. De 4 à 8, le pompage est bien net.

**Knee** La progressivité de la réduction autour du seuil, en dB.

**Attack** La vitesse à laquelle la réduction s'installe, en millisecondes. Garde quelques millisecondes pour que le transitoire de la grosse caisse passe.

**Hold** Combien de temps la réduction reste à pleine profondeur après la chute du signal Key, en millisecondes.

**Release** La vitesse à laquelle le signal principal revient, en millisecondes. De 150 à 250 ms, il a le temps de remonter avant la grosse caisse suivante.

**Makeup** Gain ajouté après la compression, en dB.

**Mix** En dessous de 1, mélange le signal atténué avec le signal direct, pour un ducking parallèle.

**Listen** Envoie le signal Key vers les sorties à la place du signal principal, pour régler Threshold à l'oreille. Désactive-le ensuite.

## Recette

**Basse qui pompe** Câble le Kick sur les entrées 3-4 et la ligne de basse sur les entrées 1-2. Detector RMS, Threshold -20, Ratio 6, Attack 2, Release 180, Mix 1. Câble aussi le Kick vers le Mixer par son propre câble, puis ajuste Release jusqu'à ce que la basse remonte juste avant la grosse caisse suivante.

## Organisms associés

SideKick, Kick, Microdot
