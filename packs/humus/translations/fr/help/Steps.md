# Steps

Un séquenceur de déclenchements à une rangée et seize pas, qui envoie une note MIDI avec swing, gate et vélocité.

Une rangée, c'est une voix : règle sa Note et câble sa sortie MIDI dans un Kick, un Mineral ou un Sampler, puis empile plusieurs organisms Steps, un par élément de batterie, chacun avec son propre swing et son propre gate. Il reste silencieux tant que le transport ne joue pas, et les pas sont lus à partir de la position en temps du transport, donc il tombe toujours en rythme avec le patch. Un coup dure Gate d'un pas, sauf si la rangée de liaison le prolonge sur les pas qui suivent. Rendre muet le câble d'une rangée, ou son canal sur le Mixer, sert de coupure en live.

## La grille

Clique sur un pas pour y placer un coup et clique de nouveau pour l'effacer. La rangée ^ au-dessus monte ce coup d'une octave. La rangée de liaison en dessous lie un coup au-delà de la limite avec le pas suivant, et une suite de liaisons forme une seule note longue.

## Paramètres

**Note** La note MIDI qu'envoie chaque coup. 36 est C1, la grosse caisse habituelle.

**SwingFollow** Follow fait prendre à la rangée le groove du patch depuis le transport, au lieu de son propre bouton rotatif Swing.

**Swing** Retarde chaque deuxième double croche, jusqu'à une sensation de triolet. Utilisé seulement quand Follow est désactivé ; donne plus de swing aux charleys qu'à la grosse caisse et le groove s'ouvre.

**Gate** La longueur du coup, en fraction du pas. Les liaisons la prolongent au-delà.

**Velocity** La vélocité de chaque coup. Automatise-la pour les montées.

## Recette

**Charleys sur un Kick** Deux organisms Steps. Le premier, Note 36, frappe sur les pas 1, 5, 9 et 13 et se câble dans un Kick avec FourFloor désactivé. Le second, Note 42, frappe un pas sur deux avec Follow désactivé et Swing 0.3, et se câble dans une zone de Sampler qui contient un charley. Monte quelques pas de charley avec la rangée ^ pour les accents.

## Organisms associés

Kick, Riff, Acid
