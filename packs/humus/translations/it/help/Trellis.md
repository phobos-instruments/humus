# Trellis

Un correttore di pitch monofonico che porta una voce o un lead sulle note di una tonalità e di una scala.

Segue la fondamentale in ingresso, la aggancia alla nota consentita più vicina della Key e della Scale scelte, e sposta lì l'audio con un pitch shifter a linea di ritardo con crossfade, una voce alla volta. Speed decide quanto è veloce lo scivolamento verso la nota bersaglio: lento conserva la deriva naturale, veloce è l'effetto hard-tune. La griglia delle note viene dall'accordatura della patch, quindi un organism Tuning impostato su 19-EDO mette le maschere di scala su 19 gradi invece che su 12. Il segnale effettato porta con sé un breve ritardo. Collega in ingresso una voce mono da un SoundIn, o un Decomposer.

## Parametri

**Strength** Quanto il pitch viene tirato verso la nota bersaglio. 0 lascia l'esecuzione com'è; 1 lo aggancia del tutto alla griglia.

**Speed** Quanto velocemente il pitch scivola verso la nota bersaglio. Basso è una correzione trasparente; vicino al massimo è lo scatto robotico.

**Key** La tonica, da C a B. Clicca il riquadro e premi un tasto, su una tastiera MIDI o sul pianoforte che si apre; qualsiasi ottava imposta la stessa tonica.

**Scale** Le note su cui il pitch può atterrare: Chromatic, Major, Minor, i modi, Harmonic e Melodic Minor, entrambe le pentatoniche, Blues, Whole Tone e Hirajoshi.

**Mix** Quanto segnale corretto si sente rispetto a quello secco.

## Ricetta

**Intonazione naturale** Key sulla tonalità del brano, Scale Major o Minor, Strength 0.7, Speed 0.4, Mix 1. Per l'effetto, invece: Scale Chromatic, Strength 1, Speed 1. Collega l'uscita a un Verbatim o a un Fern, così la correzione si inserisce in uno spazio.

## Organism correlati

Decomposer, Tuning
