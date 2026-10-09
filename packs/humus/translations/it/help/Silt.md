# Silt

Un synth chiptune a tre voci che emula il MOS SID, il chip sonoro degli home computer, attraverso il core [reSID](https://github.com/daglem/reSID).

Vengono emulati ciclo per ciclo sia il 6581 originale sia la revisione 8580, più pulita, filtro analogico compreso. Le tre voci condividono un solo filtro come nell'hardware; una quarta nota ruba la voce più vecchia. Ogni voce suona la stessa forma d'onda, la stessa larghezza d'impulso e l'inviluppo a quattro stadi proprio del chip, nei suoi sedici step nativi. Il pitch viene dall'accordatura della patch, con la risoluzione dei registri di frequenza del chip, quindi anche una scala microtonale suona. La modalità Twin aggiunge un secondo chip, come facevano le schede modificate, e raddoppia ogni nota con la coppia scordata sull'immagine stereo. Suonalo con un cavo MIDI da un PianoRoll, un DNA o un MidiIn, o con la tastiera mentre il suo editor ha il focus.

## Parametri

**Model** Quale chip: il 6581, con il suo filtro caldo e capriccioso, o l'8580, con quello corretto. Al cambio il livello rientra con un breve fade, per ammorbidire il salto tra le tensioni di riposo dei due chip.

**Wave** TRI, SAW, PLS o NSE, le quattro forme d'onda del chip.

**PulseWidth** Il duty cycle dell'onda a impulsi. Si sente solo su PLS.

**Attack** Attacco dell'inviluppo nei sedici step del chip. 0 è istantaneo.

**Decay** Decadimento dell'inviluppo fino al livello di sostegno, in sedici step.

**Sustain** Il livello tenuto finché la nota è premuta, in sedici step.

**Release** Rilascio dell'inviluppo dopo il note-off, in sedici step. A 15 risuona per secondi.

**Cutoff** Dove si chiude il filtro, lungo l'escursione dei registri dell'hardware.

**Resonance** Quanto canta forte il filtro sulla frequenza di taglio, in sedici step.

**FilterMode** LOW, BAND o HIGH: passa-basso, passa-banda o passa-alto.

**Filter** Esclude del tutto il filtro, per avere le voci grezze, più brillanti e più forti.

**Ring** Ogni voce viene modulata ad anello dalla vicina, con un suono metallico e fragoroso. Si sente attraverso l'onda TRI.

**Sync** Ogni voce fa hard sync sulla vicina.

**Chips** ONE chip, o TWIN: ogni nota su entrambi i chip, ognuno in primo piano su un lato con un po' dell'altro mescolato dentro.

**TwinDetune** Di quanto è scordata la coppia twin, in cent, a cavallo della nota. Indicato come Spread.

**Chain** Fa passare l'uscita del primo chip attraverso il filtro del secondo, così il lato sinistro arriva secco e il destro passa per un secondo filtro analogico. Solo con TWIN.

**ModelB** Quale chip è il secondo, B 6581 o B 8580. Mescolare un originale caldo con la revisione pulita è buona parte del motivo per averne due.

**Level** Livello d'uscita.

**BendRange** Fin dove arriva la rotella del pitch a fine corsa, in semitoni. Due è il valore predefinito più comune; zero ignora la rotella.

## Ricetta

**Basso chiptune** Model 6581, Wave PLS, PulseWidth 0.25, Attack 0, Decay 9, Sustain 6, Release 5, FilterMode LOW, Cutoff 0.45, Resonance 8. Collega un Riff all'ingresso MIDI e aggiungi un LFO su PulseWidth a un quarto di battuta per il classico sweep dell'impulso. Passa a TWIN con TwinDetune 8 per allargarlo.

## Organism correlati

pH, Acid, Rhizome, PianoRoll
