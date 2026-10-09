# PianoRoll

Una sorgente di note in loop che suona le sue clip attraverso i suoi cavi MIDI, ovunque si trovi il transport.

Collega la sua uscita a uno strumento qualsiasi, un Rhizome, un Sampler o un MidiOut per l'hardware, e premi Riproduci; collegalo a tre strumenti insieme e un solo riff li suona tutti e tre. È un loop che colleghi nella patch, non una parte che arrangi: gli strumenti del brano portano le loro clip sulla Timeline, mentre un PianoRoll porta le sue dentro la patch. Anche un roll vuoto segue il clock, così una linea tenue scorre sulla griglia e torna all'inizio dopo l'ultima battuta prima ancora che ci sia una nota da suonare. La registrazione cattura quello che suoni qualunque sia la modalità di ricezione MIDI dell'organism, e un take intero si annulla in un solo passo.

## Il roll

Pointer seleziona e sposta, Draw crea, Scissors divide, Eraser cancella al passaggio, sui tasti da 1 a 4. Trascina la coda di una nota per ridimensionarla e trascina in verticale con Alt per la velocity; fai clic destro su una nota o sulla selezione per darle un colore, quantizzarla su una griglia, imprimerci il groove o eliminarla, e i tasti freccia spostano la selezione di un semitono o di un passo di griglia. La striscia sotto la griglia è la corsia della velocity; fai clic destro per passare a una corsia di controller o di pitch bend e disegnarci la curva. In una corsia di controller, clicca un punto per selezionarlo, Shift-clic per aggiungerne un altro e trascina con Ctrl o Cmd per racchiuderne diversi in un riquadro, poi fai clic destro per colorarli o eliminarli. Il pulsante Loop è un looper a un tocco: tocca su una clip vuota per registrare, tocca di nuovo per chiudere il loop su una battuta intera, poi ogni tocco alterna Dub e Play. Rec registra quello che suoni su qualsiasi ingresso MIDI direttamente nel roll e Q lo quantizza sulla griglia di snap. Nessuno dei due aspetta il Registra del transport, e tutti e due continuano anche con l'editor chiuso.

## Parametri

**Bars** La lunghezza della clip in battute.

**Channel** Il canale MIDI su cui escono le note.

**Mute** Silenzia l'uscita senza scollegare niente. Le note tenute vengono rilasciate quando si attiva.

**VelocityScale** Moltiplica la velocity di ogni nota in uscita. A 1 suonano come sono disegnate.

**SwingFollow** Follow fa prendere al roll il groove della patch dal transport invece dei suoi Swing e SwingUnit.

**Swing** Ritarda ogni seconda unità, fino a un andamento terzinato. Si usa solo mentre Follow è disattivato.

**SwingUnit** Il valore di nota su cui lavora Swing, 1/8 o 1/16.

**Record** Registra il MIDI dal vivo nel roll mentre è attivo, e fa partire il transport se è fermo. Non viene mai salvato attivo. Fai clic destro su Rec per mapparlo.

**Loop** Una pressione è un tocco del looper: registra, chiude il loop, sovraincide, suona. Fai clic destro su Loop per svuotare il roll o per mapparlo.

**Quantize** Aggancia le note registrate alla griglia di snap del roll. Fai clic destro su Q per mapparlo.

## Ricetta

**Un riff, tre voci** Bars 2, disegna una figura di basso di due battute e collega l'uscita a un Rhizome, a un Sampler e a un MidiOut insieme. VelocityScale 0.8, Follow attivo così la figura prende il groove della patch, e mappa Mute su un Button per togliere la linea durante un break.

## Organism correlati

MidiIn, Sequence, Sampler
