# Tuning

L'accordatura della patch: ogni organism che trasforma una nota in una frequenza chiede la risposta al Tuning.

Non suona e non manda note. I temperamenti equabili, l'intonazione naturale da rapporti esatti, la serie degli armonici e qualsiasi file di scala .scl stanno tutti in un unico browser di scale, e Map decide come i normali numeri di nota trovano il loro pitch in una scala che non ha dodici gradi. Gli strumenti nativi, Rhizome, Substrate, Wave, pH, Mineral, Acid, Microdot e Trellis, lo seguono direttamente; ai plugin ospitati l'accordatura arriva via MIDI, come descrive la riga Plugins. Il piano roll disegna sempre dodici righe per ottava, quindi con Map su Every step in una scala che non è a 12 note le sue etichette non corrispondono più ai pitch, anche se le note sono giuste.

## Raggiungere gli organism

Collega la sua uscita MIDI a uno strumento e quello strumento suona in questa accordatura. L'accordatura scende lungo i cavi MIDI, quindi una catena Tuning, DNA, Rhizome li accorda entrambi, e due organism Tuning mettono due synth in due accordature diverse allo stesso tempo. Lascialo senza cavi e accorda tutta la patch: è il caso normale e l'unico modo per raggiungere un organism senza ingresso MIDI, come Trellis.

## Parametri

**Preset** La modalità, in fondo al box. Scale file, il default, suona la scala scelta nel browser in alto, e un nuovo Tuning parte da lì su 12-TET, l'accordatura normale, che è un file di scala come gli altri. Custom EDO divide l'ottava in un numero a scelta di Divisions uguali e sbiadisce il browser, tenendo da parte la sua scala per quando torni. Le scale che prima stavano in questa lista - 12-TET, 19, 24 e 31-EDO, Bohlen-Pierce, le scale naturali 5, 7 e 17-limit, Harmonics 8-16 e Pythagorean - ora sono nel browser, identiche nota per nota. Una patch salvata su una di queste si apre ancora su quella, ne mostra il nome qui e suona come ha sempre suonato.

**Divisions** Passi uguali per ottava per Custom EDO, da 1 a 128. Uno è solo ottave, due aggiunge il tritono, e 128 è il massimo che una scala può contenere. Sbiadito in modalità Scale file, dove decide il file.

**Root** La nota MIDI fissata a RootHz. 69 è A4.

**RootHz** A che frequenza suona quella nota. 440 è lo standard.

**Map** Come un tasto trova la sua nota quando la scala non ne ha dodici. Every step suona su ogni tasto la nota successiva della scala, quindi ogni nota è raggiungibile, un'ottava di 19 note occupa 19 tasti, e cambiare scala sposta tutto. Fit to 12 keys distribuisce un periodo della scala su una normale ottava di dodici tasti e suona la nota più vicina al posto abituale di ogni tasto, così una scala naturale di sette note cade sui tasti bianchi e il tasto dell'ottava resta un'ottava vera; con più di dodici note, alcune restano senza tasto. Con una scala di dodici note le due modalità coincidono.

**File** La scala, in cima al box: il file .scl che suona la modalità Scale file. Clicca il nome per aprire il browser di scale: le scale incluse nell'app, le tue, quelle che hai portato da altrove e quelle usate di recente, ognuna con il numero di note e la descrizione, e una ricerca che legge tutto. Le frecce passano alla scala successiva e a quella precedente. Scegliere una scala seleziona anche il preset. Un file mancante viene segnato in ambra e suona come un normale 12-TET.

**Plugins** Come l'accordatura arriva ai plugin ospitati. MTS SysEx manda messaggi di accordatura per singola nota ai plugin che conoscono lo standard. Pitch bend mette ogni nota su un canale tutto suo, con un bend per la differenza: funziona quasi ovunque, ma limita la polifonia e si prende la rotella del pitch. Auto (probe) prova ogni plugin una volta e si ricorda il verdetto.

## Ricetta

**Quinta senza battimenti** Preset Just - 17-limit, Map Fit to 12 keys, nessun cavo, così lo segue tutta la patch. Tieni una quinta su un Rhizome da un PianoRoll: si aggancia senza battimenti, come nessuna quinta temperata riesce a fare. Poi passa a Harmonics 8-16 e suona una scala sullo stesso Rhizome per sentire la serie degli armonici in sé.

## Organism correlati

Rhizome, Trellis, Substrate
