# Riff

Un séquenceur de ligne de basse façon boîte acid, à seize pas, qui envoie des notes MIDI avec gate, accent et slide par pas.

Chaque pas porte une note, un gate, un accent et un slide : le langage de pattern de la boîte acid. Un pas accentué part avec une vélocité de 118, au-dessus du seuil d'accent d'Acid, et un pas en slide tient sa note jusqu'un peu après le note-on suivant, ce qu'Acid lit comme un glissé legato. Huit banques contiennent chacune leur propre pattern ; les boutons de banque choisissent laquelle joue et laquelle la grille affiche. Il pilote aussi n'importe quel autre instrument MIDI, alors branche aussi sa sortie MIDI sur un Sampler ou un Rhizome.

## La grille

Fais glisser dans le couloir des notes pour régler la hauteur d'un pas, ce qui l'active aussi ; fais un clic droit sur un pas pour en faire un silence. Les rangées A et S activent ou désactivent l'accent et le slide. Un slide vers la même note la lie au lieu de la refrapper. Fais un clic droit sur les rangées A ou S pour Aléatoire, Effacer, Décaler, Charger des riffs et Copier vers la banque. Charger des riffs ouvre un navigateur sur ton dossier de bibliothèque Riffs, avec ceux que tu as déjà chargés sous Récentes, et Ouvrir un fichier pour tout le reste. Le riff que tu choisis, ou un fichier déposé sur la grille, lit les fichiers MIDI et les dumps de patterns de boîte acid dans la banque affichée et dans les suivantes.

## Paramètres

**Transpose** Décale tout le pattern en demi-tons.

**SwingFollow** Follow fait prendre au pattern le groove du patch depuis le transport, au lieu de son propre bouton Swing.

**Swing** Retarde une double croche sur deux, jusqu'à une sensation de triolet. Utilisé seulement quand Follow est désactivé.

**Gate** Longueur d'une note sans slide, en fraction du pas.

**Velocity** Vélocité des notes simples, non accentuées.

**Bank** Lequel des huit patterns joue, de A à H.

**Nudge** Joue le riff autant de pas plus tard, ou plus tôt en dessous de zéro, en bouclant. Le pattern lui-même reste tel qu'il est écrit : tu déplaces ainsi l'endroit où une ligne tombe par rapport au kick sans la redessiner.

## Recette

**Ligne acid** Branche la sortie MIDI sur un Acid. Dessine une fondamentale sur les pas 1, 4, 7, 11 et 14, une octave au-dessus sur 8 et 16, accentue les pas 1 et 11, mets un slide de 7 vers 8. Gate 0.5, Swing 0.2 avec Follow désactivé. Baisse Transpose de 5 pour le breakdown.

## Organisms associés

Acid, Steps, Microdot
