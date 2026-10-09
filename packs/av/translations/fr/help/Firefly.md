# Firefly

Un flash sur l'image, sur le temps, à la main ou déclenché par des notes.

Une entrée vidéo et une sortie : ce qui entre passe à travers, et le flash se pose par-dessus. Sans rien de câblé en entrée, le flash tombe sur du noir, donc Firefly seul est une lampe. Il se déclenche sur la grille du transport, à sa propre cadence libre, depuis le bouton Flash, ou à partir des notes qui arrivent sur son entrée MIDI, donc une MIDI Track, DNA ou Riff sur la timeline le pilote comme n'importe quoi d'autre. La vélocité de la note règle la force du flash. Hue et Saturation lui donnent une couleur, et comme ce sont deux paramètres ordinaires, un LFO sur Hue fait tourner la couleur pendant que tu joues. Chaque flash est minuté par l'horloge propre à l'image, donc un flash plus court qu'une image s'affiche quand même. Un avertissement à prendre au sérieux : un clignotement rapide peut déclencher des crises chez les personnes atteintes d'épilepsie photosensible. Garde-le lent devant un public que tu ne connais pas, et préviens avant.

## Paramètres

**Mode** Ce qui le déclenche tout seul. Beat se déclenche sur la grille du transport et suit le morceau, au repos quand le transport est arrêté. Free tourne à la cadence de Rate, que le morceau joue ou non. Played ne se déclenche jamais tout seul : seuls le bouton Flash et les notes sur l'entrée MIDI le font, donc les lumières suivent tes mains ou une MIDI Track et rien d'autre. Le bouton et les notes déclenchent dans tous les modes.

**Division** La fréquence des flashs en mode Beat par rapport au temps : de la ronde à la triple croche, plus les triolets et les valeurs pointées.

**Rate** Flashs par seconde en mode Free, d'une pulsation lente à 0.05 Hz jusqu'à 30 Hz.

**Width** La durée de chaque flash, en part de l'intervalle entre deux flashs ; en mode Played, l'intervalle est d'un temps au tempo du morceau. Court, c'est un flash d'appareil photo ; large, ça reste allumé aussi longtemps qu'éteint.

**Hue** La couleur du flash, en degrés autour du cercle chromatique. Elle n'apparaît que si Saturation est montée.

**Saturation** À quel point le flash est coloré. 0 % donne du blanc, 100 % la teinte pleine.

**Level** À quel point le flash couvre l'image. 100 % la remplace pendant la durée du flash.

**Flash** Déclenche un flash à la main. Assigne-le à un pad ou à une pédale et joue des lumières.

## Recette

**Flash sur le temps par-dessus une vidéo** Câble un VideoPlayer vers Firefly et Firefly vers un VideoOut. Mode Beat, Division 1/4, Width 12 %, Shape 40 %, Saturation 0 %. Lance la lecture et l'image claque à chaque temps.

**Joué depuis la timeline** Règle Mode sur Played, câble une MIDI Track dans l'entrée MIDI de Firefly et écris une ligne de notes. Les notes fortes flashent fort, les douces se voient à peine, donc les lumières lisent la partie que tu as écrite.

## Organisms associés

VideoFX, Lumen, VideoOut
