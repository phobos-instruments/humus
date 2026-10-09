# Chorus

Un chorus stéréo qui épaissit un son avec une copie retardée qui bouge lentement.

Une copie de l'entrée est retardée de quelques millisecondes et ce retard est maintenu en mouvement : la copie dérive un peu au-dessus et au-dessous de la hauteur de l'original, comme deux musiciens qui ne jouent jamais exactement à la même hauteur. Le balayage est un triangle, et celui du canal droit suit celui du gauche avec un décalage de RightLFOPhaseOffset, ce qui ouvre l'image. Rate et DelaySweepDepth agissent l'un sur l'autre : la vitesse de variation du retard fixe jusqu'où la hauteur se déplace, donc doubler l'un ou l'autre la déplace davantage. Câble-le après un Rhizome, un Wave ou un Sampler ; un Flanger, c'est la même idée avec des retards plus courts, un Phaser la même idée sans aucun retard.

## Paramètres

**MinDelay** Le retard le plus court jamais atteint, en millisecondes. Les petites valeurs tirent vers le flanger, les plus grandes restent clairement du chorus.

**DelaySweepDepth** Jusqu'où le retard monte au-dessus de MinDelay, en millisecondes. Plus de profondeur, c'est plus de désaccord.

**Rate** La vitesse à laquelle le retard se déplace, en Hz. Lent, c'est une houle ; rapide, c'est un vibrato.

**RightLFOPhaseOffset** Le retard du balayage du canal droit sur celui du gauche, en degrés. À zéro, les deux côtés se désaccordent ensemble et le résultat reste centré ; à 90 ou 180, ils divergent et l'image s'ouvre.

**Feedback** Renvoie la copie retardée vers l'entrée. Un peu de feedback ajoute de la résonance ; une valeur négative inverse le signal à chaque tour et creuse le son au lieu de l'épaissir.

**HFRolloffFrequency** Assombrit la copie retardée par le haut, en Hz. Garder le chemin traité plus terne que le direct évite qu'un chorus sonne vitreux.

**Stereoness** Réservé à la largeur de l'image traitée. Le chorus actuel garde la copie de chaque canal de son propre côté, donc ce bouton rotatif n'a pas encore d'effet audible.

**WetDryMix** La part du son retardé qu'on entend face à l'original. Tout en signal traité, le désaccord n'a plus rien de stable à quoi se comparer.

## Recette

**Nappe large** MinDelay 8, DelaySweepDepth 6, Rate 0.3, RightLFOPhaseOffset 180, Feedback 0, HFRolloffFrequency 8000, WetDryMix 0.5. Câble à l'entrée un accord tenu de Rhizome et les deux côtés s'écartent en dérivant autour du centre.

## Organisms associés

Flanger, Phaser, Fern, StereoTool
