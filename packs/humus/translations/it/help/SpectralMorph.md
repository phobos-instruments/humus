# SpectralMorph

Fonde gli spettri di due ingressi, così un timbro si trasforma nell'altro invece di suonarci sopra.

Banda per banda il livello viene interpolato tra la sorgente A e la sorgente B, e la fase viene presa da quella che lì contribuisce di più; poi il risultato viene risintetizzato. In modalità Stereo gli ingressi 1 e 2 sono A e il 3 e il 4 sono B; in modalità Mono l'ingresso 1 è A e l'ingresso 2 è B, quindi ricontrolla i cavi dopo aver cambiato Mode. Con B scollegato, Morph diventa un fade verso il silenzio. Le sorgenti tenute e ricche di armonici si prestano meglio al morphing: metti un Substrate contro una voce del Sampler e muovi Morph con un LFO o un Metapad.

## Parametri

**Morph** 0 è solo A, 1 è solo B, e tutto quello che sta in mezzo è una fusione spettrale dei due.

**A** Tienilo premuto per passare dritto ad A ovunque sia Morph; rilascialo e la fusione torna. Mappalo su un pad per un taglio spettrale.

**B** Tienilo premuto per passare dritto a B. Tenendoli premuti entrambi arrivi a metà.

## Ricetta

**Dalla voce al pad** Collega una vocale cantata da SoundIn agli ingressi 1 e 2 e un accordo di Substrate agli ingressi 3 e 4. Collega un LFO a Morph con un periodo di otto battute, così la voce si trasforma nel pad e torna indietro. Fai suonare entrambe le sorgenti per tutto lo sweep.

## Organism correlati

SpectralFilter, SpectralFreeze, SoundSpace, Mixer
