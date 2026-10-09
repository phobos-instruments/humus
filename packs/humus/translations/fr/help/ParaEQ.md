# ParaEQ

Un égaliseur à quatre bandes avec un plateau à chaque extrémité et deux bandes balayables au milieu.

Les deux plateaux relèvent ou coupent tout ce qui se trouve au-delà de leur fréquence charnière, donc ils inclinent tout le son ; les deux bandes médiums agissent sur une zone autour de leur centre, donc elles vont chercher une seule chose pour la déplacer. Les quatre s'enchaînent en série sur chaque canal. Branche-le après un Sampler, un FilePlayer ou un bus de Mixer pour enlever la boue d'une basse, pour baisser une fréquence qui résonne dans un enregistrement de pièce, ou pour ajouter de l'air à quelque chose de terne. Une petite coupure là où deux parties se battent fait en général plus qu'une accentuation sur l'une ou l'autre. La largeur de bande est donnée en hertz, donc 200 Hz de large font 200 Hz de large où que se trouve la bande, même si la même largeur couvre une plus grande part de l'octave dans le grave.

## Paramètres

**HSCutoffFrequency** La fréquence charnière du plateau aigu. Tout ce qui est au-dessus bouge ensemble.

**HSGain** La coupure ou l'accentuation du plateau aigu, en dB.

**BP1CenterFrequency** Le centre de la première bande médium.

**BP1Bandwidth** La largeur de la première bande, en Hz. Étroite pour une seule résonance, large pour un choix de couleur.

**BP1Gain** La coupure ou l'accentuation de la première bande, en dB. En négatif, elle creuse une encoche.

**BP2CenterFrequency** Le centre de la deuxième bande médium.

**BP2Bandwidth** La largeur de la deuxième bande, en Hz.

**BP2Gain** La coupure ou l'accentuation de la deuxième bande, en dB.

**LSCutoffFrequency** La fréquence charnière du plateau grave. Tout ce qui est en dessous bouge ensemble.

**LSGain** La coupure ou l'accentuation du plateau grave, en dB.

## Recette

**Trouver une résonance** Règle BP1Gain sur 12 et BP1Bandwidth sur 60, puis balaie BP1CenterFrequency jusqu'à ce que le son empire. C'est la fréquence : règle BP1Gain sur -6 à cet endroit et élargis la bande jusqu'à ce que la coupure cesse de sonner comme un trou.

## Organisms associés

Filter, Prism, SpectralFilter
