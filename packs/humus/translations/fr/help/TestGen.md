# TestGen

Un signal de test prévisible : une sinusoïde à une seule fréquence, ou un bruit blanc, rose ou brun.

Sers-t'en pour vérifier un chemin de signal, pour régler les niveaux avant qu'il y ait de la musique, ou pour entendre la forme d'un filtre seul : une sinusoïde montre ce qu'un Filter fait à une fréquence, et le bruit contient toutes les fréquences à la fois et montre ce qu'il fait à toutes. Il n'a pas d'entrée et envoie le même signal sur les deux sorties, donc un seul organism alimente une chaîne stéréo. Une sinusoïde pure se lit bien plus haut sur un vumètre qu'elle ne sonne, alors fie-toi à un VuMeter plutôt qu'à tes oreilles.

## Paramètres

**Amplitude** Niveau de sortie. Commence bas ; une sinusoïde à pleine échelle dans une chaîne inconnue, c'est comme ça qu'on abîme des enceintes.

**Frequency** Hauteur de la sinusoïde, de 10 Hz à 20 kHz. Ignoré quand une forme d'onde de bruit est choisie.

**Waveform** Sine, c'est une seule fréquence et rien d'autre. White noise a une énergie égale par hertz, Pink noise une énergie égale par octave, et Brown noise décroît encore plus vite, pour un grondement grave.

## Recette

**Vérifier un filtre** Waveform White noise, Amplitude 0.3. Branche-le dans un Filter, le Filter dans un Spectrum, puis vers un SoundOut. Balaie le Frequency du Filter et le Spectrum dessine la courbe que le filtre applique.

## Organisms associés

Wave, Rhizome, VuMeter, Spectrum
