# SpectralFilter

Un sculpteur spectral qui incline l'équilibre tonal et rend le spectre plus net ou plus lisse par rapport à sa propre enveloppe.

Là où un EQ applique une courbe fixe, SpectralFilter mesure l'entrée bande par bande et met chacune à l'échelle par rapport à l'enveloppe spectrale locale. Tilt est une pente de brillance à un seul bouton rotatif, et Contrast est un compresseur spectral : sous zéro, il rapproche les pics de leur entourage ; au-dessus de zéro, il les fait ressortir. La phase n'est pas touchée, donc à Tilt 0 et Contrast 0 il est transparent, et le chemin direct est retardé d'autant pour que n'importe quel Mix reste cohérent en phase. Le sélecteur Mode choisit Stereo ou Mono. Il ajoute de la latence. Branche-le après un Substrate, un Sampler ou tout un Mixer.

## Paramètres

**Tilt** Une pente de gain sur toute la bande, jusqu'à environ 18 dB dans chaque sens. Sous zéro, il assombrit en remontant les graves et en coupant les aigus ; au-dessus de zéro, il éclaircit.

**Contrast** Jusqu'où chaque bande est éloignée de l'enveloppe spectrale ou rapprochée d'elle. En négatif, il lisse et délave un son chargé ; en positif, il fait ressortir les pics au-dessus des creux pour un résultat plus net et plus articulé.

**Mix** La part du signal sculpté que l'on entend face au signal direct.

## Recette

**Présence sans boost** Tilt 0, Contrast 0.3, Mix 1 sur une nappe terne venant d'un Substrate. Ce sont les bandes qui culminent déjà qui remontent, et non une fréquence fixe, donc la nappe gagne en définition sans nouvelle résonance. Branche un LFO sur Contrast pour une lente respiration spectrale.

## Organisms associés

SpectralFreeze, SpectralMorph, Filter, Console
