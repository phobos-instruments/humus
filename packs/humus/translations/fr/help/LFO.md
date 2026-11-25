# LFO

Un oscillateur basse fréquence qui émet une onde comme signal de contrôle pour les boutons rotatifs des autres organisms.

Il sort Offset plus Amplitude fois l'onde à la vitesse Rate, ajouté à tout ce qui arrive sur son entrée, pour qu'un LFO puisse onduler autour de la valeur d'un Number. Avec Sync activé, un cycle dure SyncBeats temps du transport au lieu de tourner à Rate. FN joue une forme à toi : relie un Math à la prise Function et le LFO joue la formule du Math comme son onde, ou relie un Wave et il joue la table du Wave. La façade dessine un cycle de l'onde qu'il émet, avec une tête de lecture qui la parcourt, ou sur FN son mouvement récent. Fais un clic droit sur n'importe quel paramètre et Contrôler avec propose le LFO deux fois : wave est l'onde elle-même et phase est la rampe montante du cycle. Relie sa sortie par un câble à la prise d'un Gain pour un trémolo, ou au Rate d'un autre LFO pour un mouvement qui dérive. Il ne transporte pas d'audio.

## Paramètres

**Rate** Cycles par seconde quand Sync est désactivé, d'un cycle toutes les cent secondes jusqu'à 1000 Hz. Le haut de la plage atteint les fréquences audio. Sur FN, c'est aussi rate dans la formule, donc un mouvement qui avance de rate*dt va à son allure.

**Waveform** SIN, TRI, SQR, SAW, DSAW (une dent de scie descendante), S&H, qui saute à une nouvelle valeur aléatoire une fois par cycle, ou FN, la formule ou la table transmise par Function.

**Amplitude** Jusqu'où l'onde va de chaque côté d'Offset.

**Offset** Le centre autour duquel l'onde bouge. Monte-le pour garder une ondulation bipolaire en positif.

**Sync** Cale un cycle sur le transport au lieu de Rate. Pendant que le transport joue, le cycle démarre sur le temps 1 et suit chaque saut, boucle et déplacement de la tête de lecture ; quand il est arrêté, l'onde continue de tourner au tempo.

**SyncBeats** Le nombre de temps que dure un cycle quand Sync est activé. Un temps est une noire, donc 0.5 est une croche et 4 une mesure à quatre temps.

**Function** Relie ici un Math pour transmettre sa formule au LFO. Le câble porte la formule et les boutons X, Y, Z et W du Math plutôt qu'une valeur : le LFO passe sur FN, suit chaque modification tant que le câble est là et garde ce qu'il a quand on le retire. Dans la formule, phase est le cycle du LFO et rate son Rate, donc les presets Nature d'un Math bougent à l'allure du LFO. Un Wave relié transmet plutôt sa table : le LFO joue les frames à la Position du Wave et suit ce bouton.

## Recette

**Trémolo synchronisé** Sync activé, SyncBeats 0.5, Waveform SIN, Amplitude 0.4, Offset 0.6. Relie la sortie par un câble à la prise Gain d'un Gain qui porte une nappe et le niveau pulse en croches ; passe à SQR pour un gate franc.

**Balle qui rebondit** Charge le preset Nature Gravity dans un Math, relie le Math à la prise Function du LFO et la sortie du LFO à la coupure d'un filtre. Le timbre suit une balle qui rebondit de plus en plus tôt et de plus en plus bas jusqu'à s'arrêter, puis elle est relancée ; Rate accélère la balle et le X du Math la rend plus vive.

**Onde qui erre** Écris `walk := walk*(1 - rate*dt) + sqrt(6*rate*dt)*noise(); sin(tau*ph(rate*(1 + 0.15*x*walk)))` dans un Math et relie-le à Function : un sinus au Rate du LFO dont la vitesse dérive comme une main sur le bouton, sans jamais tout à fait se répéter.

## Organisms associés

Math, Number, Follower, Sig
