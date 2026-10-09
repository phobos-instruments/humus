# Prism

Un resynthétiseur harmonique qui découpe l'entrée en seize harmoniques et donne un fader à chacune.

À l'unité, une harmonique passe telle qu'on l'entend ; à zéro, elle est retirée ; au-dessus de l'unité, elle est renforcée, et cela vaut aussi pour les harmoniques que la source n'avait pas. Une harmonique ajoutée suit l'enveloppe propre de la fondamentale, donc elle s'épanouit et meurt avec la note. Refract reconstruit le son un partiel à la fois en s'appuyant sur un détecteur de hauteur, et demande un matériau à hauteur définie, une note à la fois ; Filter est un égaliseur harmonique qui agit sur le spectre lui-même et accepte les accords, les percussions et le bruit. Le chemin direct est retardé d'autant, donc n'importe quel dosage de Mix reste cohérent en phase. L'affichage montre la note sur laquelle le détecteur est calé, nommée selon le Tuning du patch. Nourris-le avec une voix, une ligne de basse ou un Wave.

## Paramètres

**Mode** Refract resynthétise les partiels en mono et descend au silence quand rien de tonal n'est entendu. Filter remodèle les harmoniques déjà présentes, stéréo intacte, et ne peut pas en ajouter de nouvelles.

**Dispersion** Refract seulement. Étire l'écart entre les partiels pour qu'une harmonique tombe au-delà de son multiple entier de la fondamentale ; en positif, on glisse vers les cloches et le métal, et une touche de négatif resserre l'empilement.

**Residual** Filter seulement. Le gain de tout ce qui se trouve entre les harmoniques. À 0, un son est réduit à sa partie tonale ; avec les faders à 0 et Residual monté, la partie tonale disparaît et il reste le souffle et l'ambiance de la pièce.

**Mix** Direct contre traité.

**Level** Niveau de sortie du signal traité.

**Harm_1** Le fader de la première harmonique, de 0 à 2 avec l'unité au milieu, et ainsi de suite pour Harm_2 à Harm_16.

## Recette

**Une cloche à partir d'une basse** Mode Refract, et branche un Rhizome qui joue des notes seules. Harm_1 sur 1, Harm_2 à Harm_5 sur 1.6, le reste sur 0, Dispersion 6, Mix 1. Chaque note sonne maintenant comme une cloche qui suit l'enveloppe propre de la basse.

## Organisms associés

Wave, Decomposer, Tuning
