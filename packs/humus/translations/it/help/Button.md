# Button

Un pulsante da premere sotto forma di organism collegabile, con un'uscita di controllo e una nota MIDI.

Premilo e l'uscita passa da Off a On, così qualsiasi manopola nella patch può seguirlo con un cavo di controllo o un collegamento Controlla con, e una nota esce dall'uscita MIDI. Rilascialo e tornano indietro entrambi. Latch trasforma la pressione in un interruttore, e così diventa un mute alla buona, un cambio di scena o un modo per tenere aperto un Gate senza tenerci sopra il dito. Mappa un pad, un tasto o un pedale su Press con il MIDI learn e il pulsante vive sul tuo controller; collega il gate di un Follower alla presa Press e la patch lo preme per te. Tutto quello che arriva via cavo al suo ingresso viene sommato all'uscita.

## Parametri

**Press** Il pulsante vero e proprio. Momentaneo, a meno che Latch sia attivo.

**Latch** Ogni pressione inverte lo stato invece di seguire il dito.

**Off** Il livello dell'uscita mentre il pulsante è disattivato. Può essere più alto di On, o negativo per un interruttore bipolare.

**On** Il livello dell'uscita mentre il pulsante è attivo.

**Slew** Quanto ci mette l'uscita a raggiungere il nuovo livello, in millisecondi. Zero scatta subito: su un guadagno fa un clic, ma è quello che vuole un trigger.

**Note** La nota MIDI inviata mentre il pulsante è attivo: note-on quando si illumina, note-off quando si spegne.

**Velocity** La velocity di quella nota.

## Ricetta

**Interruttore kill** Latch attivo, Off 0, On 1, Slew 40. Collega l'uscita al Mute di un Gain messo sul bus della batteria e mappa Press su un pad con il MIDI learn. Un tocco taglia la batteria, il successivo la riporta, senza clic.

## Organism correlati

Slider, Number, Gate, Kick
