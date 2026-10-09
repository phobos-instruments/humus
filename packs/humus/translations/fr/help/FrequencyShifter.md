# FrequencyShifter

Ajoute un nombre fixe de hertz à chaque fréquence du signal.

Un pitch shifter multiplie chaque fréquence par le même rapport, donc les harmoniques restent alignées et le son garde son identité. Cet organism, lui, ajoute le même nombre de hertz à chacune : une série à 100, 200 et 300 décalée de 50 devient 150, 250 et 350, qui n'est plus harmonique. De petits décalages de quelques hertz donnent un miroitement lent, façon phasing, contre le signal direct ; des décalages plus grands transforment une note en cloche ou en grincement. Il fonctionne comme un décaleur à bande latérale unique, donc le résultat reste propre au lieu de sonner comme une modulation en anneau, et le même oscillateur pilote les deux canaux pour que l'image stéréo tienne. Branche-le après un Rhizome ou un Sampler, ou après un Fern pour que les répétitions s'éloignent de la note.

## Paramètres

**ShiftFrequency** Combien de hertz ajouter, en positif ou en négatif. Les décalages négatifs resserrent les partiels, et les harmoniques graves peuvent être poussées sous zéro puis remonter.

**WetDryMix** La part du signal décalé entendue face à l'original. De petits décalages contre le signal direct battent lentement ; tout en signal traité, la hauteur d'origine disparaît complètement.

## Recette

**Miroitement lent** Branche un pad tenu. ShiftFrequency 3, WetDryMix 0.5, et la copie décalée bat contre le pad direct environ trois fois par seconde. Branche un LFO sur ShiftFrequency avec une Amplitude faible pour faire dériver le battement.

## Organisms associés

Prism, SpectralMorph, Chorus
