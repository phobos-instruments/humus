# MidiPlayer

Un lecteur de partitions qui charge un fichier MIDI, joue ses seize canaux par ses propres sorties audio et transmet la partition par une sortie MIDI, pour que d'autres instruments puissent la jouer aussi.

Il lit les partitions .mid et .midi, les fichiers karaoké .kar et le .mus, le format de partition compact des premiers jeux de tir à la première personne, et n'a besoin de rien d'autre de câblé : choisis un fichier et appuie sur Play. Par défaut, les voix sont en modulation de fréquence, une petite banque de réglages d'opérateurs dans la tradition de l'ordinateur domestique et de la carte son, et c'est pour ça qu'il sonne comme il sonne ; dépose une banque échantillonnée dans l'emplacement Bank et il joue des enregistrements à la place. Host tempo cale la partition sur le transport ; désactivé, il la laisse suivre sa propre carte de tempo. Câble les sorties audio dans un Fern ou une Console. La sortie MIDI porte les notes, programmes, bends et contrôleurs que contient le fichier : un synthé câblé dessus joue la même partition avec sa propre voix ; un canal coupé y est muet aussi, et l'arrêt relâche toutes les notes. Voir FilePlayer pour l'audio enregistré et MidiIn pour une partition qui arrive de l'extérieur.

## Paramètres

**File** La partition à jouer. Clique sur l'emplacement pour en choisir une, ou glisses-y un fichier.

**Bank** Les instruments que jouent les canaux. La banque de puce .wopn livrée avec l'organism est en modulation de fréquence ; une banque échantillonnée .sf2 remplace les puces par des enregistrements et ignore Chips.

**Play** Lance et arrête la partition, tout comme le bouton Play de la bande de transport ; chaque bouton de cette bande est un paramètre, alors fais un clic droit sur n'importe lequel pour Apprentissage MIDI, Apprentissage OSC, Contrôler avec ou Suivre. L'arrêt coupe toutes les notes qui sonnent ; appuyer sur Play une fois la partition terminée la relance depuis le début. Il tourne sur sa propre horloge, donc le Stop du transport le laisse tranquille ; appuie une deuxième fois sur Stop, quand plus rien ne tourne, et il s'arrête avec tout le reste.

**Loop** Revient au début quand la partition se termine, au lieu de se taire.

**FollowHost** Host tempo : activé, le tempo du transport pilote la partition ; désactivé, la partition garde le tempo auquel elle a été écrite, y compris les changements notés par le compositeur.

**Chips** Le nombre de puces sonores qui jouent en même temps, de une à quatre. Une seule, c'est six voix, et un arrangement chargé perd des notes ; quatre en donnent vingt-quatre.

**Level** Niveau de sortie.

**Program1** L'instrument que joue le canal 1. 0 joue ce que demande le fichier ; toute autre valeur le remplace pour ce canal. La même chose pour Program2 à Program16. Le canal 10 est le canal de batterie et reste de la batterie.

**Mute1** Rend muet le canal 1, sur les sorties audio comme sur la sortie MIDI, et ainsi de suite pour Mute2 à Mute16. Un canal sur lequel le fichier chargé ne joue jamais s'estompe et ne répond plus : ce qui reste allumé, c'est ce que le fichier contient vraiment.

## Recette

**Partition d'époque** Charge un .mid, Host tempo désactivé, Chips 4, Loop activé, Level 0.7, et appuie sur Play. Coupe le canal que tu veux jouer toi-même et câble les sorties à travers un Fern réglé sur un écho slapback court.

**Prête-lui une voix** Charge une partition, coupe le canal que tu veux remplacer et câble la sortie MIDI dans un synthé. Le fichier joue tel qu'il est écrit, avec une partie chantée par quelque chose à toi.

## Organisms associés

FilePlayer, MidiIn, pH
