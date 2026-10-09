# Graft

Un assembleur d'échantillons qui découpe jusqu'à huit fichiers audio en fragments et enchaîne un fragment par emplacement pour en faire un nouvel échantillon.

Charge des fichiers audio dans les emplacements, choisis comment ils sont découpés et comment les emplacements choisissent leurs fragments, et le résultat assemblé devient l'instrument : il joue depuis l'entrée MIDI, avec C4 à sa vitesse d'origine, ou bourdonne tout seul avec Play. Chaque coupe se cale sur le passage par zéro le plus proche et les fragments voisins se chevauchent sur un fondu enchaîné à puissance constante, si bien qu'une jointure entre des enregistrements sans rapport passe sans clic. La même Seed reconstruit toujours le même résultat, donc un patch enregistré se rouvre tel qu'il était, et Aléatoire en tire un nouveau sans toucher aux autres réglages. Relie un PianoRoll ou un DNA à l'entrée MIDI, ou active Play et relie la sortie à un Fern ou à un Filter.

## La carte

Chaque bande a la couleur du fichier dont elle vient, comme l'emplacement au-dessus. Clique sur une bande pour entendre ce fragment seul. Fais glisser vers le haut ou vers le bas pour le réaccorder, fais glisser sur le côté pour passer d'un fragment à l'autre du même fichier, et maintiens Shift en faisant glisser sur le côté pour passer d'un fichier à l'autre. Clic droit pour inverser l'emplacement ou le rendre muet, refaire son tirage à lui seul, ou le garder. Un emplacement gardé conserve son fragment pendant que tout le reste autour est tiré de nouveau.

## Paramètres

**File1** Un fichier source à découper, et ainsi de suite de File2 à File8. N'importe quel fichier audio.

**Slices** Le nombre d'emplacements du résultat, de 4 à 64.

**Order** La façon dont les emplacements choisissent. Random prend n'importe quel fragment de n'importe quel fichier ; In order et Round robin parcourent les fichiers chargés emplacement par emplacement ; Blocks donne à chaque fichier une suite d'emplacements contigus ; Mirror construit la première moitié au hasard et la replie à l'envers ; One source ne réarrange que le fichier désigné par Only.

**Cut** D'où viennent les fragments. Free donne à chaque fichier autant de morceaux qu'il y a d'emplacements, Grid coupe sur une valeur de note du tempo du transport, Onsets coupe sur les transitoires.

**Div** La longueur des fragments en mode Grid, sous forme de valeur de note.

**Length** En mode Free, la part de chaque fragment naturel qui est prise, de 10 à 200 pour cent. Court, ça bégaie ; long, ça se chevauche.

**Sense** En mode Onsets, combien de transitoires comptent comme des coupes.

**Xfade** Le chevauchement à chaque jointure, en millisecondes.

**Seed** Affiché comme Creature. Le même nombre reconstruit toujours le même résultat.

**Bars** 0 laisse le résultat à la longueur qu'il a en sortant. Tout autre nombre le rééchantillonne pour qu'il tienne sur ce nombre de mesures au tempo actuel, ce qui décale la hauteur jusqu'à une octave ; la ligne d'état indique de combien.

**Normalize** Remonte le résultat final juste sous la pleine échelle.

**Source** Affiché comme Only : le fichier qu'utilise One source.

**Play** Fait bourdonner le résultat en continu, sans note. Les modifications le reconstruisent en dessous sans interruption.

**Attack** L'attaque de l'enveloppe en millisecondes, pour chaque note.

**Decay** Le déclin de l'enveloppe en millisecondes.

**Sustain** Le niveau de maintien de l'enveloppe.

**Release** Le relâchement de l'enveloppe en millisecondes.

**Loop** Une note tenue tourne en boucle au lieu de s'arrêter à la fin.

**Reverse** Joue depuis la fin.

**Speed** La vitesse de lecture, d'un quart à quatre fois, sans déplacer le clavier.

**Start** L'endroit du résultat où une note commence. Fais glisser la bande sous la carte pour le placer.

**Level** Le niveau de sortie.

**BendRange** La portée de la molette de pitch en fin de course, en demi-tons. Deux est la valeur par défaut habituelle ; zéro ignore la molette.

**Voices** Le nombre de notes qui peuvent sonner en même temps, jusqu'à 16.

**SliceEdits** Les réaccordages, inversions et mises en sourdine par emplacement faits sur la carte, enregistrés avec le patch.

**Pins** Les emplacements gardés, enregistrés avec le patch.

## Recette

**Break recousu** Charge une boucle de batterie, une voix et un enregistrement de terrain dans trois emplacements. Cut Onsets, Sense 0.5, Slices 16, Order Random, Bars 2, Play activé. Appuie sur Aléatoire jusqu'à ce qu'un emplacement tombe bien, fais un clic droit dessus et garde-le, puis relance le tirage jusqu'à ce que le reste colle. Fais passer la sortie par un Fern.

## Organisms associés

Leafcutter, Sampler, Paulstretch, PianoRoll
