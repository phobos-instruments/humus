# Wave

Un oscillatore wavetable i cui cicli si disegnano, si fanno crescere da un file seme o si estraggono a caso.

Il display è il suono: disegnaci sopra, parti dalla miniatura della sinusoide, dell'onda triangolare, del dente di sega o dell'onda quadra, o genera la tabella da un file qualsiasi. Una tabella contiene fino a sedici frame e Position fa il morphing tra i due più vicini; Warp rimodella il ciclo, Unison sovrappone copie scordate, e una sinusoide sub, il drive e un passa-basso completano la voce. La riproduzione è a banda limitata per ogni frame, quindi le note alte perdono solo le armoniche che non ci stanno. È uno strumento MIDI a otto voci che segue il Tuning della patch; collega in ingresso un PianoRoll, un DNA o uno Steps e manda l'uscita a un Filter o a un Fern.

## Il display

Trascina sull'onda per disegnare il frame più vicino a Position; il resto di una tabella generata da un seme non cambia, quindi un'onda cresciuta si può ritoccare frame per frame. Casuale estrae a caso una nuova distribuzione di parziali. Trascina un file sul display o usa Da file: un file wavetable, un ciclo di 2048 campioni per frame come lo condivide la maggior parte dei synth, viene letto frame per frame, e uno lungo viene sfoltito a sedici frame tenendo il primo e l'ultimo; una registrazione viene tagliata in un massimo di sedici frame, un ciclo per slice; e qualsiasi altro file viene letto come una forma. + frame e - frame costruiscono una tabella a mano, e la striscia sotto il display mostra tutti i frame con quello corrente evidenziato; clicca una slice per saltarci. Lo stesso seme fa crescere sempre la stessa tabella, salvata con la patch.

## Parametri

**Attack** Quanto ci mette una nota ad aprirsi, in millisecondi.

**Release** Quanto ci mette a sfumare dopo che lasci il tasto.

**Sub** Una sinusoide pura un'ottava sotto ogni voce, per dare peso senza cambiare il ciclo disegnato. 0 è disattivato.

**Unison** Quante copie scordate suona ogni voce, da 1 a 16. Con una sta esattamente al centro; con di più il suono si ispessisce e si allarga, e oltre otto il costo è in CPU più che in suono.

**Detune** Quanto distano di pitch le copie in unisono.

**Width** Quanto si allargano le copie in unisono nel campo stereo.

**Cutoff** Un passa-basso sulle voci sommate. Lasciato in cima alla sua corsa esce del tutto dal percorso.

**Reso** Risonanza di quel passa-basso.

**Drive** Saturazione sulla somma, prima del filtro, così Cutoff doma quello che produce. 0 è pulito.

**WarpMode** Il modellatore del ciclo: Off, Sync piega la lettura rendendola più veloce dentro il periodo, Bend inclina la rampa di fase, Fold ripiega i picchi entro i limiti, PWM stringe il duty.

**Warp** Profondità del warp. È smussato, quindi muoverlo non crea mai scalini udibili, e il risultato modellato viene disegnato come una seconda traccia sopra il ciclo di base.

**Position** Scorre tra i frame della tabella facendo il morphing. Non fa nulla su una tabella con un solo frame.

**Spectral** Fa il morphing in base all'ampiezza delle armoniche invece che sui campioni, così una parziale che è in opposizione di fase tra due frame sfuma invece di cancellarsi a metà sweep.

**PositionMod** Quanto l'inviluppo d'ampiezza sposta Position da solo, voce per voce, così una nota viaggia attraverso la tabella mentre suona. 0 è disattivato.

**Level** Livello d'uscita.

**BendRange** Fin dove arriva la rotella del pitch a fine corsa, in semitoni. Due è il valore predefinito più comune; zero ignora la rotella.

**Table** La tabella stessa, salvata con la patch. Randomizza la estrae a caso insieme alle manopole di modellazione, e Annulla ti riporta l'onda che avevi.

## Ricetta

**Tabella campionata che evolve** Da file con una breve registrazione di voce, Unison 3, Detune 0.25, Width 0.7, Cutoff 6000, Drive 0.2, Release 800. Collega in ingresso un PianoRoll e un LFO su Position con un periodo di due battute, così ogni nota viaggia attraverso la registrazione mentre suona; attiva Spectral se lo sweep ha dei vuoti o si sfasa.

## Organism correlati

Rhizome, Prism, Tuning
