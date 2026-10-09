# Siren

Una sirena da sound system: un oscillatore ruvido che va avanti e indietro tra due pitch seguendo una rampa lenta, e suona finché la tieni.

È un nostro progetto ispirato alle classiche scatolette a due transistor, con gli stessi quattro controlli sul pannello frontale, i due estremi del pitch e le due metà dello sweep, e gli interruttori che quelle scatolette hanno sul fianco. Tieni premuto il pulsante Hold, un pad mappato su di esso o una nota qualsiasi su un cavo MIDI; ogni pressione fa ripartire lo sweep dall'inizio e quando rilasci il suono si spegne nel tempo di Release. L'uscita è mono. Il misuratore segue lo sweep, e anche lo sweep e il gate sono valori di controllo, quindi un Siren può pilotare una luce o un Filter mentre ulula.

## Parametri

**PitchLow** Il fondo dello sweep in Hz. Impostalo sopra PitchHigh e lo sweep va nell'altra direzione.

**PitchHigh** La cima dello sweep in Hz.

**LfoUp** Quanto ci mette la rampa a salire, in secondi. Up e Down diversi danno l'ululato sbilenco.

**LfoDown** Quanto ci mette la rampa a scendere, in secondi.

**Mode** Sweep sale e scende. Rise sale e torna giù di scatto, poi si ferma per il tempo di Down. Fall scende e torna su di scatto, e si ferma per il tempo di Up. Step salta tra i due pitch, la classica bitonale.

**Wave** Square, Pulse o Saw per l'oscillatore. Square è la scatoletta originale.

**Tone** Un passa-basso in uscita. Abbassalo per una tromba più cupa.

**Auto** Attivo, la rampa va da sola. Disattivato, il pitch lo imposti a mano con Manual.

**Manual** Dove sta il pitch tra PitchLow e PitchHigh quando Auto è disattivato, così con una manopola mappata ululi a mano.

**Hold** Fa suonare la sirena finché è premuto.

**Release** Quanto in fretta si spegne il suono quando lasci, in millisecondi.

**Level** Guadagno d'uscita.

## Ricetta

**Tromba bitonale** Mode Step, Wave Square, PitchLow 400, PitchHigh 530, LfoUp 0.4, LfoDown 0.4, Tone 3000. Mappa Hold su un pad, fai passare l'uscita per un Fern con una ripetizione corta e sincronizzata, e premi per due battute all'arrivo di un drop.

## Organism correlati

Kick, LFO, Button
