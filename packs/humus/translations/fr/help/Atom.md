# Atom

Quatre anneaux de frappes réparties régulièrement, qui arpègent les notes que tu tiens, ou parcourent une gamme quand tu n'en tiens aucune.

Chaque anneau répond à une question : quand. Il prend un nombre de pas et un nombre de frappes et répartit les frappes autour du cercle aussi régulièrement que possible, ce qui est la règle derrière un très grand nombre des rythmes de danse du monde : trois frappes sur huit, c'est le tresillo, cinq sur huit le cinquillo, sept sur douze la cloche standard d'Afrique de l'Ouest. Quelle note sonne, ce sont tes mains qui le décident. Branche un clavier ou un PianoRoll à l'entrée et chaque frappe joue la note suivante de l'accord que tu tiens ; ne tiens rien et les anneaux parcourent tout seuls la tonalité et la gamme choisies. Les notes que tu joues sont d'abord ramenées sur la gamme, donc rien ne tombe hors de la tonalité. Il tourne pendant que le transport joue, et envoie tout par une seule sortie MIDI.

La façade montre les anneaux de l'extérieur vers l'intérieur, l'anneau 1 étant le plus à l'extérieur. Les points allumés sont des frappes, les points creux des silences, et le point le plus brillant est le pas en train de jouer.

Les presets se répartissent en trois familles. Rhythms, ce sont les motifs de danse ci-dessus. Elements traite les anneaux comme des couches électroniques : l'anneau le plus intérieur est la première couche, chaque anneau est aussi long que sa couche peut contenir d'électrons (2, 8, 18 et 32 pas, de l'intérieur vers l'extérieur), ses frappes sont les électrons qu'elle contient, et chaque couche fait un tour par mesure, si bien que les atomes plus lourds se remplissent de couches plus denses et plus rapides. Planets fait des anneaux des orbites dont les longueurs gardent les proportions des vraies : huit tours de l'anneau de la Terre contre treize pour Vénus, les douze de Jupiter contre les vingt-neuf de Saturne, Neptune qui fait trois tours pendant que Pluton en fait deux, si bien que les anneaux s'écartent puis se retrouvent comme le font les planètes.

## Paramètres

**Steps** Combien de pas compte l'anneau, de 1 à 32.

**Hits** Combien de ces pas sonnent. L'anneau les espace aussi régulièrement qu'il peut. 0 rend l'anneau muet.

**Turn** Fait tourner toute la figure autour de l'anneau, un pas à la fois, pour que le même rythme parte d'un autre endroit.

**Rate** La durée d'un pas : une noire, une croche, une double croche ou une triple croche. Fit bar étire plutôt tout l'anneau sur une mesure, quel que soit son nombre de pas, et c'est ce qui crée une vraie polyrythmie : un anneau de 3 et un anneau de 4, tous deux sur Fit bar, jouent trois contre quatre. Des anneaux sur une valeur de note avec des nombres de pas différents continuent simplement leur route et se décalent les uns par rapport aux autres, pour ne se retrouver qu'après de nombreuses mesures.

**Fix** Fait jouer à l'anneau sa propre Note à chaque frappe au lieu de l'arpège, pour piloter une batterie ou une cloche.

**Note** La note que joue un anneau en mode Fix.

**Order** Le chemin que suit l'arpège à travers les notes : Up, Down, Up-down, Random, ou As played, qui garde l'ordre dans lequel tes doigts se sont posés. Chaque anneau garde sa propre place sur le chemin.

**Octaves** Répète les notes sur ce nombre d'octaves, vers le haut.

**Key** La tonique de la gamme que parcourent les anneaux.

**Scale** La gamme que parcourent les anneaux quand aucune note n'est tenue, et celle sur laquelle les notes tenues sont ramenées. Chromatic laisse les notes tenues telles quelles.

**Latch** Garde l'accord après que tu as lâché les touches. Le prochain accord que tu joues le remplace.

**Gate** La part d'un pas que dure chaque note.

**Vel** La vélocité de chaque note.

**Swing** Le swing propre à cet organism, utilisé quand Follow est désactivé. Un anneau sur Fit bar n'a jamais de swing.

**Follow** Prend le swing du transport ; désactive-le pour le régler ici avec Swing.

## Recette

**Un arpégiateur** Branche un clavier sur Atom et Atom sur un synthé. Laisse l'anneau 1 à 5 frappes sur 16, règle Order sur Up-down et Octaves sur 2, appuie sur Lire et tiens un accord. Monte Hits pour une ligne plus chargée, et utilise Turn pour déplacer les accents.

**Trois contre quatre** Règle l'anneau 1 sur 4 pas et 4 frappes et l'anneau 2 sur 3 pas et 3 frappes, tous deux sur Fit bar avec Fix activé et deux notes différentes, et branche la sortie sur un Sampler. Ajoute l'anneau 3 en arpège par-dessus.

**Un motif de cloche** Un anneau de 12 pas et 7 frappes sur Fit bar donne la longue cloche d'une grande partie de la musique d'Afrique de l'Ouest et centrale. Mets dessous un second anneau de 12 avec 4 frappes pour la pulsation des danseurs, et fais tourner la cloche jusqu'à ce qu'elle tombe juste.

## Organisms associés

Steps, Sequence, Riff, DNA, PianoRoll
