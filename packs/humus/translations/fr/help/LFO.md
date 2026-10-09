# LFO

Un oscillateur basse fréquence qui émet une onde comme signal de contrôle pour les boutons rotatifs des autres organisms.

Il sort Offset plus Amplitude fois l'onde à la vitesse Rate, ajouté à tout ce qui arrive sur son entrée, pour qu'un LFO puisse onduler autour de la valeur d'un Number. Avec Sync activé, un cycle dure SyncBeats temps du transport au lieu de tourner à Rate. La façade dessine un cycle de l'onde qu'il émet, avec une tête de lecture qui la parcourt. Fais un clic droit sur n'importe quel paramètre et Contrôler avec propose le LFO deux fois : wave est l'onde elle-même et phase est la rampe montante du cycle. Relie sa sortie par un câble à la prise d'un Gain pour un trémolo, ou au Rate d'un autre LFO pour un mouvement qui dérive. Il ne transporte pas d'audio.

## Paramètres

**Rate** Cycles par seconde quand Sync est désactivé, d'un cycle toutes les cent secondes jusqu'à 1000 Hz. Le haut de la plage atteint les fréquences audio.

**Waveform** SIN, TRI, SQR, SAW, DSAW (une dent de scie descendante) ou S&H, qui saute à une nouvelle valeur aléatoire une fois par cycle.

**Amplitude** Jusqu'où l'onde va de chaque côté d'Offset.

**Offset** Le centre autour duquel l'onde bouge. Monte-le pour garder une ondulation bipolaire en positif.

**Sync** Cale un cycle sur le transport au lieu de Rate. Pendant que le transport joue, le cycle démarre sur le temps 1 et suit chaque saut, boucle et déplacement de la tête de lecture ; quand il est arrêté, l'onde continue de tourner au tempo.

**SyncBeats** Le nombre de temps que dure un cycle quand Sync est activé. Un temps est une noire, donc 0.5 est une croche et 4 une mesure à quatre temps.

## Recette

**Trémolo synchronisé** Sync activé, SyncBeats 0.5, Waveform SIN, Amplitude 0.4, Offset 0.6. Relie la sortie par un câble à la prise Gain d'un Gain qui porte une nappe et le niveau pulse en croches ; passe à SQR pour un gate franc.

## Organisms associés

Number, Follower, Sig
