# Kick

Un kick de club synthétisé, avec une enveloppe de hauteur, un déclin du corps, un clic de bruit et un étage de saturation.

Une sinusoïde sur Tune démarre plus haut de Punch et redescend pendant PitchDecay, son corps s'éteint pendant Decay, une courte rafale de bruit ajoute le transitoire et un étage de saturation arrondit le tout : le kick est conçu, pas échantillonné. Il se déclenche de trois façons qui se combinent : le bouton Hit, n'importe quel note-on sur l'entrée MIDI venant d'un PianoRoll ou d'un Steps, et 4/4, qui le déclenche à chaque temps du transport pendant la lecture. Relie la sortie par un câble à un Compressor ou à une Console, et un Follower sur cette sortie à un Gain pour un ducking en sidechain.

## Paramètres

**Tune** La fréquence de repos du corps, de 30 à 90 Hz. La case à côté du bouton rotatif l'affiche sous forme de note : clique dessus et appuie sur une touche, sur un clavier MIDI ou sur le piano qui s'ouvre, pour accorder le kick sur cette note.

**Punch** De combien la hauteur démarre au-dessus de Tune. Plus il y en a, plus l'attaque est dure.

**PitchDecay** P.Dec : la vitesse à laquelle la hauteur redescend jusqu'à Tune, en millisecondes. Court, c'est un tic ; long, c'est un laser.

**Decay** Déclin du corps en millisecondes, la longueur du boom.

**Click** Niveau du transitoire de bruit, pour percer un mix chargé.

**Wood** Un court choc de deux partiels inharmoniques qui suivent Tune, disparu en 35 ms environ. Un peu de Wood ajoute un caractère de mailloche, beaucoup tire vers un wood-block. 0 laisse le son brut.

**Drive** Saturation. Pousse-la pour un grondement distordu.

**Level** Niveau de sortie.

**FourFloor** 4/4 : déclenche le kick à chaque temps pendant que le transport joue.

**Trigger** Hit : déclenche le kick une fois. Assigne-le à un pad.

## Recette

**Four on the floor** Tune 48, Punch 4, PitchDecay 40, Decay 420, Click 0.35, Drive 0.3, 4/4 activé. Lance la lecture et le kick suit le transport ; relie un Follower à sa sortie par un câble, puis la sortie env du Follower à un Gain sur la basse pour un ducking.

## Organisms associés

SideKick, Microdot, Steps
