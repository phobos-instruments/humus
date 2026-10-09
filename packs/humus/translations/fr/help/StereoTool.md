# StereoTool

Un outil d'image stéréo mid/side avec des réglages de largeur, de rotation, de basses mono et de phase.

Le mid, c'est ce que les deux canaux partagent, et le side, ce qu'ils ne partagent pas ; séparer un signal en ces deux parties permet à Width et MonoBass de changer l'image elle-même, pendant que les autres réglages la déplacent ou en règlent le niveau. L'ordre est fixe : PhaseL, PhaseR et Swap d'abord, puis l'étage mid/side où Width et MonoBass agissent sur le side, puis Rotation, Balance, Mono et Output. L'oscilloscope de l'éditeur dessine le champ et sa corrélation, donc une source qui va disparaître en mono se voit avant que tu l'entendes. Branche-le avant une Console ou un SoundOut, ou sur un retour de Send.

## Paramètres

**Width** Niveau du side. 0% replie l'image en mono, 100% la laisse telle quelle, et il va jusqu'à 400%. Au-dessus de 100%, il amincit le centre en élargissant, donc vérifie avec Mono pendant que tu le règles.

**Balance** Équilibre de niveau entre gauche et droite, le réglage habituel façon panoramique.

**Rotation** Fait tourner tout le champ jusqu'à 45 degrés dans un sens ou dans l'autre. Contrairement à Balance, il déplace l'image au lieu d'en régler le niveau, donc le centre se déplace avec tout le reste.

**MonoBass** En dessous de cette fréquence, le side est retiré et le grave se replie en mono. 0 le désactive ; 100 à 150 Hz est le réglage habituel.

**PhaseL** Inverse le canal gauche. Inverse un côté quand une source stéréo sonne creux et disparaît en mono.

**PhaseR** Inverse le canal droit.

**Swap** Échange la gauche et la droite.

**Mono** Somme la sortie en mono. Sers-t'en pour vérifier ce que fait Width au-dessus de 100%.

**Output** Gain de sortie, appliqué en dernier.

## Recette

**Élargir sans casser le mono** MonoBass 120, Width 140%, Balance et Rotation à 0. Active et désactive Mono en écoutant ; si le centre s'amincit trop en mono, ramène Width vers 120%. Règle Width après MonoBass, pas avant, sinon tu juges une largeur que le passage en mono des graves va reprendre.

## Organisms associés

Console, Mixer, Gain, SoundSpace
