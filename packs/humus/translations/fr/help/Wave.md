# Wave

Un oscillateur à table d'ondes dont les cycles se dessinent, poussent à partir d'un fichier de base ou se tirent au hasard.

L'affichage, c'est le son : dessine dessus, pars de la vignette sinus, triangle, dent de scie ou carré, ou fais pousser la table à partir de n'importe quel fichier. Une table contient jusqu'à seize frames et Position fait un morphing entre les deux plus proches ; Warp remodèle le cycle, Unison empile des copies désaccordées, et un sinus sub, une saturation et un passe-bas complètent la voix. La lecture est à bande limitée pour chaque frame, donc les notes aiguës ne perdent que les harmoniques qui ne tiennent pas. C'est un instrument MIDI à huit voix qui suit le Tuning du patch ; câble en entrée un PianoRoll, un DNA ou un Steps et envoie la sortie vers un Filter ou un Fern.

## L'affichage

Fais glisser sur l'onde pour dessiner la frame la plus proche de Position ; le reste d'une table issue d'un fichier de base reste intact, donc une onde qu'on a fait pousser peut se retoucher frame par frame. Aléatoire tire une nouvelle répartition de partiels. Dépose un fichier sur l'affichage ou utilise Fichier de base : un fichier de table d'ondes, un cycle de 2048 échantillons par frame comme la plupart des synthés les échangent, est lu frame par frame, et un fichier long est réduit à seize frames qui gardent sa première et sa dernière ; un enregistrement est découpé en seize frames au plus, un cycle par tranche ; et tout autre fichier est lu comme une forme. + frame et - frame construisent une table à la main, et la bande sous l'affichage montre toutes les frames avec l'actuelle éclairée ; clique sur une tranche pour y sauter. La même base fait toujours pousser la même table, enregistrée avec le patch.

## Paramètres

**Attack** Le temps que met une note à s'ouvrir, en millisecondes.

**Release** Le temps qu'elle met à s'éteindre une fois la touche relâchée.

**Sub** Un sinus pur une octave sous chaque voix, pour du poids sans changer le cycle dessiné. 0 le désactive.

**Unison** Combien de copies désaccordées joue chaque voix, de 1 à 16. Une seule reste pile au centre ; davantage épaississent et élargissent, et au-delà de huit le coût se paie en CPU plus qu'en son.

**Detune** L'écart de hauteur entre les copies à l'unisson.

**Width** L'étalement des copies à l'unisson dans le champ stéréo.

**Cutoff** Un passe-bas sur les voix sommées. Placé tout en haut de sa course, il sort complètement du trajet du signal.

**Reso** Résonance de ce passe-bas.

**Drive** Saturation sur la somme, avant le filtre, pour que Cutoff dompte ce qu'elle produit. 0 est propre.

**WarpMode** Le façonneur de cycle : Off, Sync accélère la lecture à l'intérieur de la période, Bend incline la rampe de phase, Fold replie les crêtes dans la plage, PWM comprime le rapport cyclique.

**Warp** Profondeur du warp. Il est lissé, donc le balayer ne crée jamais de grésillement, et le résultat façonné est dessiné comme une seconde trace par-dessus le cycle de base.

**Position** Parcourt les frames de la table en faisant un morphing entre elles. Sans effet sur une table d'une seule frame.

**Spectral** Fait le morphing selon l'amplitude des harmoniques plutôt que selon les échantillons, donc un partiel en opposition de phase entre deux frames s'estompe au lieu de s'annuler en plein balayage.

**PositionMod** De combien l'enveloppe d'amplitude balaie Position d'elle-même, voix par voix, pour qu'une note voyage dans la table pendant qu'elle joue. 0 le désactive.

**Level** Niveau de sortie.

**BendRange** Jusqu'où va la molette de pitch bend à fond de course, en demi-tons. Deux est la valeur par défaut la plus courante ; zéro ignore la molette.

**Table** La table elle-même, enregistrée avec le patch. Aléatoire la tire en même temps que les boutons rotatifs de façonnage, et Annuler ramène l'onde que tu avais.

## Recette

**Table évolutive à partir d'un enregistrement** Fichier de base avec un court enregistrement de voix, Unison 3, Detune 0.25, Width 0.7, Cutoff 6000, Drive 0.2, Release 800. Câble un PianoRoll en entrée et un LFO sur Position avec une période de deux mesures, pour que chaque note voyage dans l'enregistrement pendant qu'elle joue ; active Spectral si le balayage se creuse ou sonne en phasing.

## Organisms associés

Rhizome, Prism, Tuning
