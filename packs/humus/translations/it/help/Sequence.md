# Sequence

Una griglia di batteria a sedici step, da una a otto righe, in cui ogni riga ha la sua uscita MIDI.

Ogni riga manda le sue note dalla propria uscita MIDI, così la riga 1 può pilotare un Kick, la riga 2 un Sampler e la riga 3 un Silt, e la targhetta sulla riga mostra a cosa è collegata. L'ultima uscita è il master e porta tutte le righe insieme, per un unico campionatore di batteria che mappa l'intero kit per nota. Otto banchi contengono ciascuno una griglia completa; le note, le velocity e le abilitazioni delle righe sono il kit e restano uguali in tutti i banchi. Il pattern è una battuta di sedicesimi e le modifiche hanno effetto anche mentre il transport suona.

## La griglia

Clicca uno step per attivarlo, clicca di nuovo per cancellarlo, trascina per dipingerne una serie. La colonna in corso si illumina mentre il transport suona. Il LED a sinistra della riga è la sua abilitazione, la manopola è la sua velocity, e la rotella del mouse sulla targhetta cambia la nota della riga. Fai clic destro su uno step per Casuale, Cancella e Copia nel banco, per copiare la griglia in un altro banco. Fai clic destro su uno step, sul LED o sulla targhetta per Escludi la riga dalla randomizzazione: ogni Casuale, compreso quello della griglia, lascia allora com'erano gli step e la velocity di quella riga, e sulla targhetta compare un lucchetto. Per tenere solo una velocity, fai clic destro sulla sua manopola e scegli Escludi dalla randomizzazione, come su qualsiasi manopola.

**Probabilità dello step** Clicca l'interruttore % sotto Gate per vedere la probabilità di ogni step acceso come una barra, poi trascina su o giù su uno step per decidere quanto spesso suona, da mai a sempre, a scatti del 5%. Cmd-trascina uno step (Ctrl-trascina su Windows e Linux) fa lo stesso senza cambiare modalità. L'interruttore % è il parametro View, quindi anche un pulsante o un pad mappato lo commuta. La manopola accanto alla velocity di ogni riga è la Chance della riga: vale per tutti gli step di quella riga, così un solo fader dirada una linea di hi-hat. Fai clic destro sulla manopola, o su uno step della riga nella vista Chance, per mapparla. Uno step sotto il 100% appare più sbiadito e mostra la sua probabilità su di sé. I dadi sono del box: una patch suona gli stessi step a ogni riproduzione e a ogni bounce, e due box tirano ciascuno i propri. Ogni banco tiene le sue probabilità, Copia nel banco le porta con sé e Cancella le riporta al sicuro.

## Parametri

**SwingFollow** Follow fa prendere alla griglia il groove della patch dal transport invece della sua manopola Swing.

**Swing** Ritarda i sedicesimi in levare, da dritto a 0 fino a uno shuffle del tutto terzinato a 1. Si usa solo mentre Follow è disattivato.

**Gate** La lunghezza della nota come frazione di uno step.

**Enable_1** Se la riga 1 suona; gli step restano mentre è disattivata. E così via per le righe da 2 a 8.

**Vel_1** La velocity delle note della riga 1. E così via per le righe da 2 a 8.

**Note_1** La nota MIDI che manda la riga 1, di default secondo la mappa di batteria standard, 36 per una cassa. Le batterie sintetiche come Kick la ignorano; i campionatori la mappano. E così via per le righe da 2 a 8.

**Bank** Quale delle otto griglie suona, da A a H.

**View** Steps o Chance: cosa mostra e modifica la griglia. L'interruttore % sotto Gate lo cambia.

**Chance_1 a Chance_8** Quanto spesso suonano gli step di ogni riga, oltre alla probabilità di ogni step.

**Rows** Quante righe ha la griglia, da 1 a 8. Le uscite seguono: una per riga, e il master sempre per ultimo, così un cavo sul master resta sul master quando il numero cambia. Le righe che togli conservano step, note e velocity, e tornano com'erano se le riaggiungi; vengono rimossi solo i cavi dalle loro uscite.

## Ricetta

**Cassa dritta** Collega la riga 1 a un Kick, la riga 2 a un Cicada o a un clap di un Sampler, la riga 3 a un campione di hi-hat. Step 1, 5, 9 e 13 sulla riga 1, 5 e 13 sulla riga 2, uno step sì e uno no sulla riga 3. Swing 0.15 con Follow disattivato, Gate 0.5. Disegna un fill nel banco B e cambia Bank da un Button per il turnaround.

## Organism correlati

Riff, Steps, PianoRoll
