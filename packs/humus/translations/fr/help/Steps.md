# Steps

Un séquenceur de déclenchements à une rangée et seize pas, qui envoie une note MIDI avec swing, gate et vélocité.

Une rangée, c'est une voix : règle sa Note et câble sa sortie MIDI dans un Kick, un Mineral ou un Sampler, puis empile plusieurs organisms Steps, un par élément de batterie, chacun avec son propre swing et son propre gate. Il reste silencieux tant que le transport ne joue pas, et les pas sont lus à partir de la position en temps du transport, donc il tombe toujours en rythme avec le patch. Un coup dure Gate d'un pas, sauf si la rangée de liaison le prolonge sur les pas qui suivent. Rendre muet le câble d'une rangée, ou son canal sur le Mixer, sert de coupure en live.

## La grille

Clique sur un pas pour y placer un coup et clique de nouveau pour l'effacer. La rangée ^ au-dessus monte ce coup d'une octave. La rangée de liaison en dessous lie un coup au-delà de la limite avec le pas suivant, et une suite de liaisons forme une seule note longue.

**Probabilité du pas** Clique sur la touche % en haut à droite de la grille pour voir la probabilité de chaque pas sous forme de barre, puis fais glisser vers le haut ou le bas sur un pas pour régler la fréquence à laquelle il joue, de jamais à toujours, par pas de 5 %. Cmd-glisser (Ctrl-glisser sous Windows et Linux) fait de même sans changer de mode. La touche % est l'interrupteur View, donc un bouton ou un pad assigné la bascule aussi. La probabilité de chaque pas est aussi un paramètre, de Chance_1 à Chance_16, pour qu'on puisse y assigner des faders. Dans la vue Chance, fais un clic droit sur un pas pour l'assigner. Un coup sous 100 % apparaît plus pâle et affiche sa probabilité au-dessus de lui. Les dés appartiennent à la boîte : un patch joue les mêmes coups à chaque lecture et à chaque export, et deux boîtes lancent chacune les leurs. Chaque banque garde ses probabilités, Copier vers la banque les emporte et Effacer les remet à coup sûr.

## Paramètres

**Note** La note MIDI qu'envoie chaque coup. 36 est C1, la grosse caisse habituelle.

**SwingFollow** Follow fait prendre à la rangée le groove du patch depuis le transport, au lieu de son propre bouton rotatif Swing.

**Swing** Retarde chaque deuxième double croche, jusqu'à une sensation de triolet. Utilisé seulement quand Follow est désactivé ; donne plus de swing aux charleys qu'à la grosse caisse et le groove s'ouvre.

**Gate** La longueur du coup, en fraction du pas. Les liaisons la prolongent au-delà.

**Velocity** La vélocité de chaque coup. Automatise-la pour les montées.

**Bank** Lequel des huit patterns joue, de A à H. Chaque bank garde ses propres pas et sa propre rangée ^, et changer joue le nouveau aussitôt.

**View** Steps ou Chance : ce que la grille montre et modifie. La touche % le change.

**Chance_1 à Chance_16** La fréquence à laquelle chaque pas joue, de 0 à 100 %. Chaque banque garde les siennes.

## Recette

**Charleys sur un Kick** Deux organisms Steps. Le premier, Note 36, frappe sur les pas 1, 5, 9 et 13 et se câble dans un Kick avec FourFloor désactivé. Le second, Note 42, frappe un pas sur deux avec Follow désactivé et Swing 0.3, et se câble dans une zone de Sampler qui contient un charley. Monte quelques pas de charley avec la rangée ^ pour les accents.

## Organisms associés

Kick, Riff, Acid
