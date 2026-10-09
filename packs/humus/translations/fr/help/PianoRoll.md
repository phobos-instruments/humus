# PianoRoll

Une source de notes en boucle qui joue ses clips à travers ses câbles MIDI, où que soit le transport.

Branche sa sortie sur n'importe quel instrument, un Rhizome, un Sampler ou un MidiOut pour du matériel, et appuie sur Lire ; branche-le sur trois à la fois et un seul riff pilote les trois. C'est une boucle que tu branches dans le patch plutôt qu'une partie que tu arranges : les instruments du morceau portent leurs clips sur la Timeline, tandis qu'un PianoRoll porte les siens dans le patch. Un roll vide suit quand même l'horloge, donc une ligne pâle balaie la grille et revient au début après la dernière mesure avant même qu'il y ait une note à jouer. L'enregistrement capture ce que tu joues quel que soit le mode de réception MIDI de l'organism, et une prise entière s'annule en une seule étape.

## Le roll

Pointer sélectionne et déplace, Draw crée, Scissors scinde, Eraser efface au passage, sur les touches 1 à 4. Fais glisser la fin d'une note pour la redimensionner, et fais glisser verticalement avec Alt pour la vélocité ; fais un clic droit sur une note ou sur la sélection pour lui donner une couleur, la quantifier sur une grille, lui appliquer le groove ou la supprimer, et les touches fléchées déplacent la sélection d'un demi-ton ou d'un pas de grille. La bande sous la grille est le couloir de vélocité ; fais un clic droit dessus pour passer à un couloir de contrôleur ou de pitch bend et y dessiner la courbe. Dans un couloir de contrôleur, clique sur un point pour le sélectionner, Shift-clic pour en ajouter un autre et glisse avec Ctrl ou Cmd pour en encadrer plusieurs, puis fais un clic droit pour les colorer ou les supprimer. Le bouton Loop est un looper à une seule touche : appuie quand le clip est vide pour enregistrer, appuie de nouveau pour fermer la boucle sur une mesure entière, puis chaque appui alterne entre Dub et Play. Rec enregistre ce que tu joues sur n'importe quelle entrée MIDI directement dans le roll, et Q le quantifie sur la grille de magnétisme. Aucun des deux n'attend le bouton Enregistrer du transport, et les deux continuent même avec l'éditeur fermé.

## Paramètres

**Bars** La longueur du clip en mesures.

**Channel** Le canal MIDI sur lequel sortent les notes.

**Mute** Coupe la sortie sans rien débrancher. Les notes tenues sont relâchées quand il s'active.

**VelocityScale** Multiplie la vélocité de chaque note en sortie. À 1, elles jouent telles qu'elles sont dessinées.

**SwingFollow** Follow fait prendre au roll le groove du patch depuis le transport, au lieu de ses propres Swing et SwingUnit.

**Swing** Retarde une unité sur deux, jusqu'à une sensation de triolet. Utilisé seulement quand Follow est désactivé.

**SwingUnit** La valeur de note sur laquelle Swing agit, 1/8 ou 1/16.

**Record** Enregistre le MIDI joué en direct dans le roll tant qu'il est activé, et lance le transport s'il est à l'arrêt. Un patch enregistré ne le garde jamais activé. Fais un clic droit sur Rec pour l'assigner.

**Loop** Un appui est une action du looper : enregistrer, fermer la boucle, faire un overdub, lire. Fais un clic droit sur Loop pour vider le roll ou l'assigner.

**Quantize** Cale les notes enregistrées sur la grille de magnétisme du roll. Fais un clic droit sur Q pour l'assigner.

## Recette

**Un riff, trois voix** Bars 2, dessine une figure de basse de deux mesures et branche la sortie sur un Rhizome, un Sampler et un MidiOut à la fois. VelocityScale 0.8, Follow activé pour que la figure prenne le groove du patch, et assigne Mute à un Button pour couper la ligne le temps d'un break.

## Organisms associés

MidiIn, Sequence, Sampler
