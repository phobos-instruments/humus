# Deck

Une platine DJ pour fichiers audio et vidéos : pitch fader, grille rythmique, repères, boucles et contrôle par vinyle timecode.

**Vinyle timecode** Testé et pris en charge avec le Traktor timecode vinyl MK2 et avec Phase Essential. Active Timecode, relie la platine vinyle aux entrées du Deck via un organism Phono, et le disque pilote le plateau : scratchs, backspins, et lever le diamant l'arrête. La platine reconnaît le pressage et apprend la fréquence de son signal quand le diamant se pose. Si le plateau tourne encore trop vite ou trop lentement à vitesse normale, fais tourner le disque à 33 (ou 45) tours et appuie sur Calibrate : cette fréquence devient la vitesse normale de la platine, enregistrée avec le patch, et la platine arrête de la remettre en question. Appuie sur Calibrate avec le diamant levé pour revenir à l'apprentissage automatique. Le disque peut tourner avant le début du morceau ou après sa fin : la platine y joue du silence et se recale quand le disque revient. Choisis le disque dans la liste sous Timecode (et 45 tours s'il tourne à 45) : la vitesse normale vient alors du disque lui-même, et Absolute fait suivre au morceau la position du diamant, si bien qu'un diamant posé n'importe où tombe au bon endroit du morceau et que le scratch ne peut pas dériver. La première fois qu'un disque est choisi, sa table met quelques secondes à se construire, et la platine suit la vitesse du disque en attendant. Si le disque n'est pas suivi, regarde le scope à côté de l'interrupteur : un anneau net, c'est du timecode, une ligne en diagonale veut dire que les deux canaux portent la même chose, un nuage veut dire que ce n'est pas du timecode du tout.

**Beatmatching** Charger un morceau détecte son BPM, sa grille et sa tonalité. Active Sync sur deux platines et toutes deux suivent le tempo du patch, en rythme avec chaque séquenceur et chaque delay du patch.

**Vidéos** Charge une vidéo et la platine joue son son et envoie son image par la sortie vidéo ; l'image suit le plateau, scratch compris. PictureSync recale l'image par rapport au son, en millisecondes, si ton écran est en retard ou en avance.

**Crête** Appuie sur Crête et la platine met en boucle la partie la plus forte du morceau pendant huit temps, en partant du temps qui la précède quand Quantize est activé, pour régler les niveaux sur le plus gros coup. Appuie encore pour la deuxième plus forte, et encore pour la troisième ; les trois sont marquées sur la vue d'ensemble. Le nombre à côté du bouton est l'échantillon le plus fort du morceau, et passe au rouge à pleine échelle. L'image d'une vidéo suit, donc sa scène la plus forte boucle aussi.

**La tête de lecture partout** Fais un clic droit sur n'importe quel bouton rotatif du patch : la position, la vitesse et les secondes de la platine se trouvent sous Contrôler avec.

## Paramètres

**File** Le fichier audio ou la vidéo que joue la platine.

**AudioTrack** Quelle piste audio d'une vidéo jouer, quand elle en a plusieurs.

**Record** Arme la platine : ce qui arrive à ses entrées pendant que le patch enregistre atterrit dans le dossier Recordings du projet et revient sur ce plateau.

**FileSound** Le son joué quand ce n'est pas celui contenu dans File, réglé par une prise enregistrée.

**Active** Lecture ou arrêt, avec un court fondu pour éviter le clic.

**Loop** Boucle entre LoopIn et LoopOut.

**Sync** Suit le tempo du patch et aligne les temps sur la grille.

**Keylock** Change le tempo sans changer la hauteur. Contourné quand Timecode est activé.

**PictureSync** Affiché comme Audio delay : décale l'image par rapport au son, jusqu'à une seconde dans un sens ou dans l'autre, par platine. Une valeur positive retarde le son derrière l'image ; si le son arrive après l'image, passe en négatif.

**Timestamp** Incruste le temps de la tête de lecture de la platine dans l'image qu'elle envoie.

**Quantize** Cale les repères et les boucles sur le temps de grille le plus proche.

**Timecode** Laisse un disque de contrôle piloter le plateau.

**TimecodeHz** Gardé pour que les anciens patchs s'ouvrent sans changement ; la platine ne le lit plus. TimecodeRest contient la calibration.

**TimecodeFlip** Active-le si le morceau joue à l'envers quand le disque tourne dans le bon sens.

**TimecodeRest** La fréquence du signal de contrôle à vitesse normale, réglée par Calibrate. 0 veut dire que la platine l'apprend toute seule quand le diamant se pose.

**TimecodeCalibrate** Prend la fréquence du signal qui arrive en ce moment comme vitesse normale. Sans signal, revient à l'apprentissage automatique.

**TimecodeRecord** Quel disque de contrôle est sur le plateau. Learn from the record lit la vitesse de n'importe quel disque ; le nommer donne la vitesse normale exacte et permet à Absolute de trouver le diamant.

**Timecode45** Le disque tourne à 45 tours au lieu de 33.

**TimecodeAbsolute** Le morceau suit la position du diamant sur le disque, moins l'amorce du disque. Entre deux lectures, pendant le scratch et la marche arrière, il suit la vitesse du disque.

**BPM** Le tempo du morceau.

**PitchPercent** Le pitch fader, en pourcentage.

**PitchRange** La course du fader, de 8 à 50 pour cent.

**Volume** Niveau de sortie, absent de la façade : règle le niveau sur le Mixer, ou assigne ce paramètre.

**GridOffset** L'emplacement du premier temps fort de la grille, en échantillons.

**LoopIn** Début de la boucle, en échantillons.

**LoopOut** Fin de la boucle, en échantillons.

**LoopBeats** Longueur de la boucle en temps, d'un quart de temps à 32 temps.

**Peak** Met en boucle la partie la plus forte du morceau ; chaque appui passe à la suivante des trois plus fortes.

**Cue** Le point de repère principal, en échantillons. Cue y saute sans démarrer ni arrêter la platine.

**Key** La tonalité détectée au chargement du morceau.

**HotCue_1** Le premier hot cue, en échantillons ; -1 veut dire non défini. Appuyer sur un hot cue vide mémorise la tête de lecture, appuyer sur un hot cue mémorisé lance la lecture depuis lui, Shift-clic l'efface. Et ainsi de suite de 2 à 8.

## Recette

**Mix à deux platines** Charge un morceau sur chaque Deck et vérifie que la grille tombe bien sur les temps. Active Sync sur les deux, relie-les à un Mixer, et fais entrer la seconde sur un hot cue placé sur son premier temps fort.

## Organisms associés

AudioTrack, Sampler, Mixer, Console, Phono
