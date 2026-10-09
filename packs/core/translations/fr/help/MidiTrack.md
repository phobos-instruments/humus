# MidiTrack

Un couloir de la timeline qui contient une partie MIDI et la joue dans n'importe quel instrument du patch.

Il n'a pas de son à lui ni de boîte dans le patcher : la partie vit dans le couloir, et l'étiquette de la ligne nomme l'organism dans lequel elle joue. Choisis une autre cible et les mêmes notes passent à un autre instrument, intactes : une partie peut ainsi s'écouter sur plusieurs synthés, ou rester à l'abri pendant que tu reconstruis le patch autour d'elle. Les nouvelles pistes démarrent sur (nothing) et restent muettes tant qu'aucune cible n'est choisie. Le bouton d'armement de la ligne fait passer l'entrée MIDI en direct par la piste jusqu'à la cible, et ce que tu joues atterrit dans un clip pendant l'enregistrement. Muet, solo, renommer et supprimer marchent depuis la ligne comme pour n'importe quelle autre piste.

## Paramètres

**Target** L'instrument dans lequel cette piste joue, affiché sur l'étiquette de la ligne. La liste propose chaque organism du patch qui a une entrée MIDI ; en choisir un relie par un câble la sortie MIDI de la piste à cet organism, et un câble tiré à la main depuis cette sortie règle Target de la même façon.

## Recette

**Une partie, deux synthés** Règle Target sur un Rhizome, arme la ligne et enregistre une phrase, ou dessine-la dans le clip. Ajoute un deuxième instrument, passe Target sur lui, et le même clip joue le nouveau synthé ; reviens au premier une fois que tu as comparé les deux.

## Organisms associés

MidiBus, PianoRoll, Steps
