# Grit

Un synthé chip construit sur la puce sonore de console Ricoh 2A03, avec un emplacement de cartouche pour les puces d'extension de la Famicom.

La console seule, ce sont deux voix pulse avec les quatre rapports cycliques matériels, un triangle placé sous la note tenue la plus grave, un canal de bruit sur les touches du haut et un échantillonneur delta 1 bit sur les touches du bas. Insère une cartouche : ses voix rejoignent les autres et prennent les notes en premier, et les pulses de la console s'y ajoutent quand elles sont toutes occupées ; sur le VRC6, c'est la dent de scie qui passe en premier. L'enveloppe compte en frames par palier de volume sur seize niveaux, donc une attaque rapide claque et une lente monte de façon audible. L'émulation est celle de [NSFPlay](https://github.com/bbbradsmith/nsfplay), pilotée registre par registre. Câble un PianoRoll, un Riff ou un DNA sur l'entrée MIDI.

## Paramètres

**Cartridge** La puce dans l'emplacement. 2A03 est la console seule ; VRC6 ajoute deux pulses et une dent de scie, MMC5 deux pulses, FDS une voix à table d'ondes, N163 quatre voix à table d'ondes, 5B trois carrés fixes et VRC7 six voix FM à deux opérateurs. La changer coupe les notes tenues.

**Region** Console NTSC ou PAL. L'horloge, les tables de hauteur et le pas de l'enveloppe suivent, donc PAL tourne un peu plus lentement et sonne plus sombre.

**Duty** Largeur d'impulsion des pulses de la console, du VRC6 et du MMC5 : 12%, 25%, 50% ou 75%.

**Attack** Frames par palier de volume en montée. 0 fait démarrer une note à plein volume.

**Decay** Frames par palier de volume en descente jusqu'à Sustain.

**Sustain** Le niveau où se stabilise une note tenue, de 0 à 15.

**Release** Frames par palier de volume une fois la touche relâchée.

**Triangle** Tri layer : place le canal triangle de la console sous la note tenue la plus grave, à la même hauteur. Il n'a pas de réglage de volume, seulement activé ou désactivé.

**NoiseKeys** Noise keys : à partir de C7, les touches jouent le canal de bruit au lieu d'un son tonal, en parcourant les seize périodes matérielles à mesure que tu montes.

**Buzz** Passe le bruit dans son mode à boucle courte, un cliquetis métallique au lieu d'un souffle.

**Wave** La table d'ondes que jouent FDS et N163 : SIN, TRI, SAW, SQR ou ORG.

**Patch** L'un des quinze instruments FM intégrés du VRC7, par numéro.

**Sample** Un son pour le canal delta, réduit à 1 bit. Les touches sous C2 le déclenchent aux seize vitesses de lecture matérielles, de plus en plus grave en descendant depuis B1.

**DpcmLoop** Loop : fait tourner l'échantillon en boucle tant que sa touche est tenue.

**Level** Niveau de sortie.

**BendRange** Jusqu'où va la molette de pitch bend à fond de course, en demi-tons. Deux est la valeur par défaut la plus courante ; zéro ignore la molette.

## Recette

**Lead de cartouche** Cartridge VRC6, Duty 25%, Attack 0, Decay 6, Sustain 10, Release 3, Tri layer activé. Câble un Riff sur l'entrée MIDI pour la mélodie et ajoute en dessous un Kick avec 4/4 activé. Dépose un court coup de batterie dans Sample et joue-le à partir de B1 sur un PianoRoll.

## Organisms associés

pH, Silt, Riff
