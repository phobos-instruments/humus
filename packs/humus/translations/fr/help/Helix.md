# Helix

Un looper live à quatre strands, avec Rec, Dub, Play et Stop sur chaque strand.

Chaque strand est sa propre boucle, jusqu'à 30 secondes. Appuie sur Rec pour enregistrer et de nouveau sur Rec pour fermer la boucle et la lire, ou sur Dub pour la fermer et continuer tout de suite à superposer des couches. Dub démarre et clôt un overdub sur une boucle en lecture, et si tu le maintiens, l'overdub ne dure que tant que tu appuies. Play et Stop ferment aussi un enregistrement, en lecture ou en silence. Chaque overdub est une couche que Undo retire et que Redo remet. Chaque strand a son propre Sync : Free ferme la boucle exactement là où tu appuies, Beat et Bar calent les appuis, ainsi que Rev et Half, sur la grille du transport, et un appui un peu en retard est ramené à la ligne de grille que tu visais. Follow les cale plutôt sur la jointure d'une boucle qui tourne déjà sur un autre strand que sur le transport, si bien qu'une première boucle jouée librement à la main devient la grille sur laquelle les autres s'alignent ; si rien d'autre ne tourne, il attend la mesure. Les entrées 1 et 2 sont l'entrée principale et alimentent chaque strand ; les entrées 3 à 10 sont une paire stéréo par strand, et un strand enregistre sa propre paire par-dessus l'entrée principale, donc une paire branchée lui donne une voix que les autres n'entendent pas. La sortie principale porte les entrées écoutées plus chaque strand audible, et les sorties 3 à 10 sont des sorties directes stéréo, une paire par strand, après son Level, sa mise en sourdine et son solo. Play all et Stop all font bouger tous les strands à la fois. Les boucles sont enregistrées avec le patch comme fichiers son dans son dossier de projet et se rouvrent à l'arrêt, prêtes pour Rec ou Play. Pour mettre des boucles sur la timeline, fais un clic droit sur l'organism et choisis Audio vers piste : un strand, ou Toutes pour chaque strand enregistré, chacun sur sa propre piste audio, à la mesure sous la tête de lecture. Tu peux aussi tirer un strand par le milieu de son anneau et le déposer sur la timeline là où tu le veux. Branche un SoundIn dans l'entrée, et assigne le Rec1 d'un strand à une pédale pour enregistrer, lire et faire des overdubs avec une seule pédale.

L'anneau de chaque strand dessine ce qui y est enregistré : la forme d'onde de la boucle elle-même, vive derrière la tête de lecture et atténuée devant, avec le nombre de couches au milieu. Un halo à l'extérieur de l'anneau suit le niveau qui arrive sur ce strand, pour que tu voies où va le son avant d'armer quoi que ce soit. Les deux flèches sous l'anneau décalent une boucle en lecture : maintiens celle de gauche et le strand tourne trois pour cent plus lentement, maintiens celle de droite et il tourne trois pour cent plus vite, et quand tu relâches, il garde la place atteinte, donc une petite touche le déplace d'un cheveu et un long appui le déplace beaucoup. C'est la solution pour une boucle un peu en avance ou en retard sur les autres. La hauteur varie légèrement pendant que tu maintiens, comme sous une main posée sur un disque.

## Paramètres

**Rec1** Le déroulé à un seul bouton pour une pédale ou un pad, absent de la façade : enregistrer, fermer et lire, overdub, lire. Maintiens-le pendant la lecture pour ne faire l'overdub que tant que tu appuies. Pareil pour les strands 2 à 4.

**Record1** Rec : lance l'enregistrement d'un strand vide, et pendant l'enregistrement, ferme la boucle et la lit.

**Dub1** Dub : lance un overdub sur une boucle en lecture ou à l'arrêt et le clôt à l'appui suivant ; maintiens-le pour ne faire l'overdub que tant que tu appuies. Pendant l'enregistrement, il ferme la boucle et passe directement en overdub.

**Play1** Relance le strand depuis le début de sa boucle, sur la grille quand Sync le demande. Il lance aussi une boucle que Stop a fermée en silence, et pendant l'enregistrement, il ferme la boucle et la lit.

**Stop1** Arrête le strand. Pendant l'enregistrement, il ferme la boucle en silence, prête à être lancée.

**Undo1** Retire la couche la plus récente. Pendant l'enregistrement, il jette la prise.

**Redo1** Remet la dernière couche retirée.

**Clear1** Vide le strand. Sur la façade, maintiens-le un instant, pour qu'un clic égaré ne puisse pas faire perdre une boucle.

**Level1** Le niveau du strand dans le mix principal et sur sa sortie directe.

**Mute1** M : rend le strand muet. Il continue de tourner en dessous, donc il revient en phase.

**Solo1** S : fait entendre ce strand seul.

**Sync1** Free, Beat, Bar ou Follow : ce sur quoi se calent les appuis, Rev et Half. Follow utilise la jointure du strand en lecture qui a le plus petit numéro, pour que sa boucle devienne la grille ; efface ce strand et le suivant en lecture prend le relais. Un nouvel Helix démarre le strand 1 sur Bar et les strands 2 à 4 sur Follow, pour que la première boucle fixe la longueur sur laquelle les autres se calent.

**Rev1** Lit le strand à l'envers. L'inverser termine un overdub en cours.

**Slice1** Le quart de la boucle du strand qui est tenu, 0 pour aucun. Clique sur un quart de l'anneau pour le tenir et relâche pour le libérer, comme les quatre pads d'un contrôleur de découpe ; le paramètre s'assigne à une pédale ou à un pad comme n'importe quel autre.

**Nudge1** -1 quand la flèche de gauche est maintenue, 1 quand c'est celle de droite, 0 sinon. Un décalage maintenu fait tourner un strand en lecture trois pour cent plus lentement ou plus vite, et le strand garde la place atteinte ; il n'a aucun effet sur un strand en enregistrement, en overdub ou à l'arrêt. Il s'assigne comme n'importe quel autre paramètre, donc une paire de pédales, ou une seule molette à rappel central, décale une boucle sans la souris.


**Half1** Lit le strand à mi-vitesse, une octave plus bas. Une couche ajoutée en overdub à mi-vitesse est relue à vitesse double quand tu le désactives.

**Shot1** Once : le strand joue un seul passage et s'arrête. Rec le relance.

**Decay** La part des anciennes couches qui survit à chaque passage d'overdub. En dessous de 1, une boucle continue d'évoluer au lieu de s'empiler.

**Monitor1** Off, Mix ou Out : où l'on entend l'entrée live du strand, c'est-à-dire l'entrée principale plus sa propre paire. Mix la fait jouer par la sortie principale, et l'entrée principale n'est entendue qu'une fois, quel que soit le nombre de strands sur Mix ; Out la fait jouer sur la sortie directe du strand, pour un strand qui va vers son propre canal. Pareil pour Monitor2 à Monitor4.

**Monitor** Le réglage d'écoute unique des anciens patchs : un strand sans Monitor propre le suit.

**Follow** Fige chaque strand, en silence, tant que le transport est arrêté, et reprend quand il tourne, les strands synchronisés repartant sur la mesure. Désactivé, les boucles continuent de tourner quoi qu'il arrive. L'enregistrement n'est jamais interrompu.

**PlayAll** Play all : relance ensemble chaque strand enregistré depuis le début de sa boucle, sur la grille la plus large que l'un d'eux utilise : la mesure quand l'un d'eux est sur Bar, le temps quand l'un d'eux est sur Beat. Les strands déjà en lecture repartent aussi, donc les boucles qui dérivent se remettent en phase. Un strand réglé sur Once qui a fini attend son propre Rec ou Play.

**StopAll** Stop all : arrête chaque strand qui tourne, et un strand encore en enregistrement ferme sa boucle en silence. Il n'attend la mesure que quand chaque strand qui tourne est sur Bar, et le temps quand aucun n'est sur Free ; un seul strand sur Free lui fait tout arrêter dès l'appui.

**Loop1** Le fichier son du strand, écrit à côté du patch quand tu l'enregistres, avec le tempo auquel la boucle a été enregistrée. Pareil pour Loop2 à Loop4.

**Tempo1** Pitch, Stretch ou Off : ce que fait le strand quand le tempo s'éloigne de celui de son enregistrement. Pitch accélère ou ralentit la boucle comme une bande, hauteur comprise. Stretch ne change que sa longueur et garde sa hauteur, et un overdub garde la hauteur à laquelle tu l'as joué ; au-delà de huit fois le tempo d'enregistrement, il joue comme Pitch. Off ignore le tempo. Pareil pour Tempo2 à Tempo4.

## Recette

**Base calée, voix libre** Sync1 Bar, Sync2 Free, Follow transport activé, chaque Monitor sur Mix, Decay 0.85. Avec le transport qui tourne et un SoundIn branché, appuie sur Rec du premier strand sur le un, joue quatre mesures et appuie sur Dub sur le un suivant pour continuer à superposer, puis de nouveau sur Dub pour clore. Appuie sur Rec du deuxième strand au milieu d'une phrase pour un strand de voix qui dérive par rapport à la grille. Branche les sorties 3 et 4 dans un Fern pour traiter le premier strand seul.

## Organisms associés

Repeater, Leafcutter, Sampler
