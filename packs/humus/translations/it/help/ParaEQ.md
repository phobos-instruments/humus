# ParaEQ

Un equalizzatore a quattro bande con uno shelving a ogni estremo e due bande regolabili nel mezzo.

I due shelving alzano o tagliano tutto quello che sta oltre la loro frequenza d'angolo, quindi inclinano tutto il suono; le due bande centrali lavorano su una zona attorno al loro centro, quindi intervengono su una cosa sola. Tutte e quattro lavorano in serie su ogni canale. Collegalo dopo un Sampler, un FilePlayer o un bus del Mixer per togliere il fango da un basso, per abbassare una frequenza che risuona in una registrazione d'ambiente, o per dare aria a qualcosa di spento. Un piccolo taglio dove due parti si pestano i piedi di solito fa più di un boost su una delle due. La larghezza di banda è data in hertz, quindi largo 200 Hz è largo 200 Hz ovunque stia la banda, anche se in basso la stessa larghezza copre una parte maggiore dell'ottava.

## Parametri

**HSCutoffFrequency** La frequenza d'angolo dello shelving alto. Tutto quello che sta sopra si sposta insieme.

**HSGain** Il taglio o il boost dello shelving alto, in dB.

**BP1CenterFrequency** Il centro della prima banda centrale.

**BP1Bandwidth** Quanto è larga la prima banda, in Hz. Stretta per una sola risonanza, larga per una scelta di timbro.

**BP1Gain** Il taglio o il boost della prima banda, in dB. In negativo è un notch.

**BP2CenterFrequency** Il centro della seconda banda centrale.

**BP2Bandwidth** La larghezza della seconda banda, in Hz.

**BP2Gain** Il taglio o il boost della seconda banda, in dB.

**LSCutoffFrequency** La frequenza d'angolo dello shelving basso. Tutto quello che sta sotto si sposta insieme.

**LSGain** Il taglio o il boost dello shelving basso, in dB.

## Ricetta

**Trovare una risonanza** Imposta BP1Gain a 12 e BP1Bandwidth a 60, poi muovi BP1CenterFrequency finché il suono non peggiora. Quella è la frequenza: lì imposta BP1Gain a -6 e allarga la banda finché il taglio non smette di sembrare un buco.

## Organism correlati

Filter, Prism, SpectralFilter
