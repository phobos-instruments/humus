# SoundSpace

Un granulateur qu'on joue en faisant voler un curseur sur une carte de grains triés selon leur son, et qui enregistre aussi ce qui lui arrive.

Charge jusqu'à quatre fichiers son ; SoundSpace les découpe en grains, analyse l'énergie, la brillance, le caractère bruité et le mouvement de chacun, et les disperse sur une carte en deux dimensions où les grains semblables atterrissent près les uns des autres, une couleur par fichier. Le curseur joue un nuage des grains qui l'entourent, donc le faire glisser sur la carte fait un morphing entre les textures. Mélange des matériaux très différents, voix, machines et enregistrements de terrain, et la carte s'étale davantage, ce qui rend le voyage plus spectaculaire. Fais un clic droit sur la carte pour automatiser X et Y ou leur appliquer l'apprentissage MIDI, pour qu'un pad XY ou un LFO lent puisse la survoler. Live est activé au départ, donc l'entrée est déjà une source à part entière et il suffit d'y brancher quelque chose : ses huit dernières secondes sont découpées et déposées sur la même carte au fur et à mesure qu'elles arrivent, en points pâles parmi les couleurs des fichiers, les plus récents plus gros, pour que tu voies où tombe ton jeu et que tu fasses voler le curseur entre lui et les enregistrements. Le vumètre à côté de Freeze montre le niveau de l'entrée : une carte vide avec un vumètre éteint veut dire que rien n'arrive. Les deux cohabitent : joue en entrée pendant que des fichiers sont chargés et la carte contient les deux, et Rec capture l'entrée dans le prochain emplacement de fichier libre, pour qu'un passage qui t'a plu devienne l'un d'eux. Live n'a besoin d'aucun fichier ; sans fichier chargé, la carte va du sombre au brillant de gauche à droite et du tonal au bruité vers le haut.

## Paramètres

**Mute** Coupe la sortie pendant que le nuage continue de voler, donc réactiver le son reprend en pleine texture.

**Record** Le pad rouge Rec. Capture l'entrée audio dans un fichier son dans Documents et le dépose dans le premier emplacement libre quand il s'arrête. Les prises silencieuses sont écartées, et une prise est limitée à deux minutes.

**Live** Granule l'entrée audio au fur et à mesure qu'elle arrive, et est activé au départ. Les huit dernières secondes restent sur la carte ; le son plus ancien disparaît, et le silence est laissé de côté. Désactive-le pour jouer seulement les fichiers chargés.

**Freeze** Fige le tampon live, pour que le nuage continue de jouer ce qu'il a capté pendant que l'entrée est ignorée. Seulement avec Live activé.

**X** La position horizontale du curseur sur la carte, de 0 à 1.

**Y** La position verticale du curseur sur la carte, de 0 à 1.

**Density** Grains par seconde.

**Scatter** L'irrégularité d'arrivée des grains. À zéro, ils arrivent exactement au rythme de Density, comme une horloge ; en montant, chaque intervalle est tiré n'importe où entre presque rien et près du double, tandis que le rythme moyen reste là où Density le place, donc le nuage trébuche et s'agglutine au lieu de faire tic-tac.

**GrainSize** La longueur des grains en millisecondes. Ouvre la plage et chaque grain tire sa propre longueur à l'intérieur.

**Fit** Met le pad à l'échelle de l'endroit où se trouvent vraiment les sons. Une banque dont les points tombent dans un coin remplit le pad au lieu de s'y cacher, et les nombres le long des bords indiquent la fenêtre plutôt que tout l'espace. Il ne zoome jamais plus près qu'un quart de l'espace, pour qu'un seul amas serré ne devienne pas un microscope, et il ne se réajuste que quand les points changent - jamais sous ta main en plein glissement. Désactivé, il donne tout l'espace, comme avant.

**Spray** Le rayon de sélection autour du curseur. Petit, il reste sur une texture ; grand, il échantillonne tout le voisinage.

**Spread** À quel point chaque grain est écarté du centre dans le panoramique, au hasard.

**Pitch** La transposition en demi-tons. Fermé, c'est un seul intervalle ; ouvre-le et chaque grain est tiré quelque part dans la plage, donc une plage uniquement vers le haut donne un miroitement montant plutôt qu'un flou symétrique.

**Skew** L'endroit où se trouve le pic dans un grain, une plage comme les autres. Fermé à zéro, c'est une cloche douce ; vers un, la montée raccourcit jusqu'à ce que le grain soit frappé et résonne, ce qui transforme une nappe en rythme, même à densité égale.

**Shape** La part d'un grain tenue ouverte à plein niveau entre sa montée et sa descente. À zéro, le grain n'est que montée et descente ; vers un, il devient un bloc à sommet plat, ce qui donne une texture plus lisse et plus continue à partir du même matériau.

**Input** Le niveau de l'entrée quand elle entre dans le tampon live, jusqu'à une fois et demie. Baisse-le pour laisser Feedback prendre le relais, monte-le pour garder le nouveau jeu au premier plan. Seulement avec Live activé.

**Feedback** La part du nuage réécrite dans le tampon live. À fond, le nuage s'alimente lui-même pour toujours et ne s'éteint jamais ; la réécriture est limitée en douceur, donc il s'épaissit et sature au lieu de s'emballer. Avec Pitch éloigné de zéro, chaque passage tombe un intervalle plus haut ou plus bas, donc une note tenue s'empile en accord. Seulement avec Live activé, et pas quand il est figé.

**Mix** L'équilibre entre l'entrée directe et le nuage. Entièrement traité par défaut.

**Quant** La force avec laquelle un grain est attiré sur la grille choisie par Sync. À zéro, il n'est pas attiré du tout ; à fond, il attend la ligne. Il ne fait rien quand Sync est sur Free.

**Decorrelation** À quel point les choix aléatoires d'un grain s'accordent entre eux. À zéro, un seul tirage décide à la fois de sa hauteur, de sa longueur, de sa tenue et de son panoramique, donc les grains plus aigus tombent d'un côté ; à fond, chacun est tiré séparément.

**Voices** Le nombre maximal de grains qui peuvent sonner en même temps, de 1 à 200. Moins amincit la texture et coûte moins ; plus l'épaissit.

**Sync** Un commutateur rotatif qui cale chaque grain sur la grille du transport : Free, d'une triple croche à une noire, ou une mesure. Density décide toujours quand un grain est dû ; Sync le fait attendre la ligne suivante. Seulement pendant que le transport joue.

**Gain** Niveau de sortie.

**File1** Le premier fichier du corpus, et ainsi de suite pour File2 à File4. L'analyse se lance quand un fichier est choisi.

## Recette

**Nappe évolutive** Charge un enregistrement de voix dans File1 et un enregistrement de terrain dans File2. Density 12, GrainSize 200, Spray 0.08, Spread 0.8, Pitch un peu ouvert de chaque côté. Branche un LFO à 0.02 Hz sur X et un autre à 0.03 Hz sur Y pour que le curseur se promène sur la carte, et envoie la sortie dans un Verbatim.

**Nuage live** Aucun fichier, Live activé, une voix ou un instrument branché sur l'entrée. Density 20, GrainSize 150, Spray 0.3, Pitch un peu ouvert. Joue une phrase, appuie sur Freeze, puis fais voler le curseur dans ce que tu viens de jouer. Pour des accords, désactive Freeze, règle Pitch sur 7 et monte Feedback à 0.6.

## Organisms associés

CameraIn, Sampler
