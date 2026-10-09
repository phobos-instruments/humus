# Board

Une carte à microcontrôleur au bout d'un câble série : ses entrées analogiques ressortent sur des sorties de contrôle et ses prises pilotent des broches de la carte.

Flashe la carte une fois avec le croquis standard de contrôle des broches, fourni en exemple avec l'éditeur de la carte, branche-la et choisis un preset pour ta carte. Chaque sortie lit un canal analogique de 0 à 1, quelle que soit la résolution de la carte : un potentiomètre ou un capteur de lumière sur A0 devient la sortie 1 ; branche-la sur un Number pour la surveiller ou sur la prise d'un Gain pour façonner un niveau. Chaque prise pilote une broche, pour l'allumer et l'éteindre ou la faire varier. La carte est lue en dehors du chemin audio, et la rebrancher la reconnecte toute seule. Board est aussi une source de contrôle : le menu du clic droit de n'importe quel paramètre propose Contrôler avec Board. Une carte sans le croquis standard a trois secondes pour démarrer, puis on lui parle quand même ; une carte qui fait tourner ton propre croquis va plutôt sur SerialIn et SerialOut.

## La ligne d'état

La ligne sous les sorties indique où en est la boîte : recherche d'un périphérique, attente du port, démarrage, dialogue en cours, broches assignées. Les sorties restent à zéro tant que la carte n'a pas démarré. Les presets fixent les canaux que lisent les sorties et les broches que pilotent les prises ; la carte signale ensuite elle-même ce que chaque broche sait faire, donc une carte absente de la liste fonctionne quand même avec des broches saisies à la main.

## Paramètres

**Device** Le périphérique série sur lequel se trouve la carte. Auto prend le premier périphérique série USB trouvé ; le reste de la liste contient tous les périphériques présents. Autant de boîtes Board, SerialIn et SerialOut que tu veux peuvent partager un même port.

**BaudRate** La vitesse du port. Le croquis standard ouvre à 57600, la valeur par défaut ici ; ne la change que si tu as modifié le croquis.

**Custom** Toute vitesse absente de la liste. Zéro laisse BaudRate décider.

**Reset** Activé, ouvrir le port redémarre la carte comme le fait son éditeur, ce qu'attend le croquis standard. Désactivé, la carte continue de tourner pendant une reconnexion.

**Reconnect** Ferme le port puis le rouvre pour toutes les boîtes qui le partagent. Utilise-le quand une carte ne répond plus.

**Rate** L'intervalle auquel la carte échantillonne et envoie ses entrées analogiques, en millisecondes. 19 est la valeur par défaut du croquis ; plus bas, c'est plus réactif et plus chargé sur le fil.

**Outlets** Le nombre de sorties affichées par la boîte, de une à six.

**Inlets** Le nombre de prises affichées par la boîte, de une à six.

**In1** La première prise, et le niveau que reçoit sa broche quand rien n'est branché dessus. Et ainsi de suite de 2 à 6.

**Analog1** Le canal analogique que lit la sortie 1, de 0 à 15. Par défaut, la sortie 1 lit A0, la sortie 2 lit A1, et ainsi de suite de 2 à 6.

**Pin1** La broche de la carte que pilote la prise 1. Et ainsi de suite de 2 à 6.

**Mode1** Digital met la broche à l'état haut quand la prise dépasse 0.5, et à l'état bas sinon. PWM règle le rapport cyclique de la broche d'après la valeur de 0 à 1 de la prise ; seules certaines broches savent faire du PWM, et les presets les placent sur les prises 2 à 6. Et ainsi de suite de 2 à 6.

**Port** Un chemin de périphérique explicite, qui remplace Device tant qu'il est renseigné. Normalement laissé vide.

## Recette

**Du capteur au filtre** Relie un potentiomètre à A0, choisis le preset de ta carte et vérifie que la ligne d'état indique talking. Fais un clic droit sur Frequency d'un Filter, choisis Contrôler avec Board out1, et le bouton rotatif suit le potentiomètre. Branche l'env d'un Follower sur In2 avec Mode2 sur PWM, et la LED de cette broche varie avec la musique.

## Organisms associés

SerialIn, SerialOut, Number, Follower
