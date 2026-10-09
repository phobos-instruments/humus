# Sequence

Una griglia di batteria a sedici step, da una a otto righe, in cui ogni riga ha la sua uscita MIDI.

Ogni riga manda le sue note dalla propria uscita MIDI, così la riga 1 può pilotare un Kick, la riga 2 un Sampler e la riga 3 un Silt, e la targhetta sulla riga mostra a cosa è collegata. L'ultima uscita è il master e porta tutte le righe insieme, per un unico campionatore di batteria che mappa l'intero kit per nota. Otto banchi contengono ciascuno una griglia completa; le note, le velocity e le abilitazioni delle righe sono il kit e restano uguali in tutti i banchi. Il pattern è una battuta di sedicesimi e le modifiche hanno effetto anche mentre il transport suona.

## La griglia

Clicca uno step per attivarlo, clicca di nuovo per cancellarlo, trascina per dipingerne una serie. La colonna in corso si illumina mentre il transport suona. Il LED a sinistra della riga è la sua abilitazione, la manopola è la sua velocity, e la rotella del mouse sulla targhetta cambia la nota della riga. Fai clic destro su uno step per Casuale, Cancella e Copia nel banco, per copiare la griglia in un altro banco.

## Parametri

**SwingFollow** Follow fa prendere alla griglia il groove della patch dal transport invece della sua manopola Swing.

**Swing** Ritarda i sedicesimi in levare, da dritto a 0 fino a uno shuffle del tutto terzinato a 1. Si usa solo mentre Follow è disattivato.

**Gate** La lunghezza della nota come frazione di uno step.

**Enable_1** Se la riga 1 suona; gli step restano mentre è disattivata. E così via per le righe da 2 a 8.

**Vel_1** La velocity delle note della riga 1. E così via per le righe da 2 a 8.

**Note_1** La nota MIDI che manda la riga 1, di default secondo la mappa di batteria standard, 36 per una cassa. Le batterie sintetiche come Kick la ignorano; i campionatori la mappano. E così via per le righe da 2 a 8.

**Bank** Quale delle otto griglie suona, da A a H.

**Rows** Quante righe ha la griglia, da 1 a 8. Le uscite seguono: una per riga, e il master sempre per ultimo, così un cavo sul master resta sul master quando il numero cambia. Le righe che togli conservano step, note e velocity, e tornano com'erano se le riaggiungi; vengono rimossi solo i cavi dalle loro uscite.

## Ricetta

**Cassa dritta** Collega la riga 1 a un Kick, la riga 2 a un Cicada o a un clap di un Sampler, la riga 3 a un campione di hi-hat. Step 1, 5, 9 e 13 sulla riga 1, 5 e 13 sulla riga 2, uno step sì e uno no sulla riga 3. Swing 0.15 con Follow disattivato, Gate 0.5. Disegna un fill nel banco B e cambia Bank da un Button per il turnaround.

## Organism correlati

Riff, Steps, PianoRoll
