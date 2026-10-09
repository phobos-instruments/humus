# Comber

Une banque de filtres en peigne accordés qui fait naître des accords de tout ce qui la traverse.

Un filtre en peigne est un delay si court, avec réinjection, qu'il sonne à une hauteur au lieu de faire écho. Comber en aligne jusqu'à huit côte à côte, cinq au départ, chacun avec sa hauteur, son déclin et son niveau, donc une batterie, du bruit ou une voix ressortent en résonnant sur les notes sur lesquelles les peignes sont accordés. Chaque peigne affiche sa hauteur de trois façons : un curseur, le nombre en hertz, et le nom de la note la plus proche avec son écart en cents. Clique sur le nom de la note pour choisir une note sur un clavier, ou tourne la molette dessus pour avancer par demi-tons. Les peignes résonnent côte à côte et sont additionnés, donc ce qui sort est un accord. Quand tu changes une hauteur, elle glisse au lieu de produire un clic, et le tout fonctionne en stéréo.

## Paramètres

**InputGain** Le niveau général, appliqué à l'entrée du son. Baisse-le quand les longs déclins commencent à s'empiler.

**Combs** Le nombre de peignes utilisés, de un à huit. Les rangées au-delà sont masquées et muettes.

**Frequency_1** Hauteur du peigne 1 en hertz, de 16 à 12000, et ainsi de suite pour les autres. Le retard dure une période de cette fréquence.

**DecayTime_1** Combien de temps le peigne 1 résonne après l'arrêt de l'entrée, de 0.02 à 10 secondes, et ainsi de suite pour les autres. Court, c'est un claquement accordé ; long, c'est une note tenue.

**Gain_1** Niveau du peigne 1, et ainsi de suite pour les autres : la part du peigne dans la somme, donc zéro le rend muet.

## Recette

**Un accord tiré d'une boucle de batterie** Câble en entrée une boîte à rythmes ou une boucle. Accorde les cinq peignes sur 110, 165, 220, 277 et 330, règle chaque déclin sur 2, le gain de chaque peigne sur 0.6 et InputGain sur 0.5. Raccourcis les déclins vers 0.3 pour un claquement accordé, ou pousse un peigne à 8 pour une note qui plane au-dessus des autres.

## Organisms associés

Fern, Verbatim, ParaEQ
