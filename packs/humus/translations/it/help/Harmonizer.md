# Harmonizer

Un harmonizer con rilevatore di intonazione che aggiunge fino a quattro copie trasposte di una singola linea melodica.

Ascolta il pitch al suo ingresso e ci costruisce intorno delle armonie, ognuna la stessa esecuzione spostata su una nuova nota. Le armonie suonano solo finché si sente un pitch chiaro, così respiri e consonanti passano puliti. In modalità MIDI sono le note tenute all'ingresso MIDI a decidere i bersagli: canta una nota e l'accordo segue la tua mano. In modalità SCALE li decidono Key, Scale e i quattro intervalli Voice, così le armonie restano in tonalità ovunque vada la melodia. Collegaci un SoundIn o un FilePlayer, e un PianoRoll o una tastiera all'ingresso MIDI per la modalità MIDI.

## Parametri

**Mode** MIDI prende i bersagli dalle note tenute all'ingresso MIDI, fino a quattro, a livello pieno. SCALE li ricava da Key, Scale e dagli intervalli Voice.

**Key** Il centro tonale per la modalità SCALE, da C a B.

**Scale** La scala in cui si contano gli intervalli, da Chromatic e Major fino a Blues, Whole Tone e Hirajoshi.

**Voice1** L'intervallo della prima voce in gradi della scala, da -7 a 7: 2 è una terza sopra, -3 una quarta sotto, 7 l'ottava. Lo stesso vale da Voice2 a Voice4.

**Level1** Il livello della prima voce in modalità SCALE; una voce a zero tace. Lo stesso vale da Level2 a Level4.

**Glide** Quanto tempo impiega una voce a raggiungere un nuovo bersaglio, in millisecondi. A 0 salta.

**Humanize** Una lenta deriva di intonazione per ogni voce, fino a circa 14 cent, così l'insieme smette di suonare come un'unica macchina.

**Spread** Apre le voci a ventaglio nel campo stereo, attorno alla linea secca.

**Mix** Crossfade dalla sola linea secca alle sole armonie. Tutto effettato, tace dove non suona nessuna armonia.

## Ricetta

**Terze diatoniche** Mode SCALE, Key sulla tonalità del brano, Scale Major, Voice1 2 con Level1 0.8, Voice3 -3 con Level3 0.5, Glide 40, Humanize 0.2, Spread 0.6, Mix 0.5. Collegaci una voce da un SoundIn e mettici dopo un Compressor per tenere compatte le voci sovrapposte.

## Organism correlati

Cluster, Prism, Trellis, Decomposer
