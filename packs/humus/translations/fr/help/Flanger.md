# Flanger

Un delay court balayé qui creuse le signal en peigne contre une copie de lui-même.

Une copie de l'entrée est retardée d'une fraction de milliseconde puis remélangée, ce qui annule certaines fréquences et en renforce d'autres en une série d'encoches régulièrement espacées. Un balayage sinusoïdal déplace ce retard entre les deux bornes de FrequencyRange, si bien que tout le peigne glisse de haut en bas dans le spectre, et Feedback renvoie la copie retardée à l'entrée pour que chaque encoche s'affûte en résonance. Utilise-le sur des batteries, des accords tenus ou du bruit, où le peigne en mouvement est souvent la seule hauteur que le son possède. Chorus retarde plus longtemps et s'entend comme un désaccord ; Phaser crée moins d'encoches, irrégulièrement espacées, en décalant plutôt la phase.

## Paramètres

**FrequencyRange** Les deux bornes du balayage, exprimées en fréquences. Une fréquence basse donne un long retard et un peigne dense, une haute un retard court et un peigne clairsemé qui siffle ; rapproche les poignées pour une couleur métallique fixe au lieu d'un balayage.

**Rate** La vitesse à laquelle le balayage passe d'une borne à l'autre, en cycles par seconde.

**Feedback** La part de la copie retardée qui revient à l'entrée. Bas, c'est un souffle doux ; haut, c'est la version résonante qui hurle.

**WetDryMix** La part de la copie retardée qu'on entend face à l'original. Le peigne n'existe que là où les deux s'entendent ensemble, donc la moitié donne l'annulation la plus profonde et le signal traité seul fait disparaître l'effet.

## Recette

**Fill de réacteur** Place-le après un organism Drums. FrequencyRange 100 à 3000, Rate 0.1, Feedback 0.7, WetDryMix 0.5. Câble un LFO sur Rate si tu veux que le balayage lui-même accélère et ralentisse au fil d'une phrase.

## Organisms associés

Chorus, Phaser, Fern
