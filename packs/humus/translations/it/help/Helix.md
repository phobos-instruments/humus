# Helix

Un looper live a quattro strand, con Rec, Dub, Play e Stop su ogni strand.

Ogni strand è un loop a sé, lungo fino a 30 secondi. Premi Rec per registrare e di nuovo Rec per chiudere il loop e farlo suonare, oppure Dub per chiuderlo e continuare subito ad aggiungere strati. Dub avvia e chiude un overdub su un loop in riproduzione, e se lo tieni premuto l'overdub dura solo finché lo tieni. Anche Play e Stop chiudono una registrazione, facendola suonare o lasciandola muta. Ogni overdub è uno strato che Undo toglie e Redo rimette. Ogni strand ha il suo Sync: Free chiude il loop esattamente dove premi, Beat e Bar agganciano alla griglia del transport le pressioni e i cambi di Rev e Half, e una pressione un po' in ritardo viene riportata sulla linea della griglia che intendevi. Follow le aggancia invece al punto di giunzione di un loop che sta già girando su un altro strand, non al transport, così un primo loop suonato a mano libera diventa la griglia su cui si allineano gli altri; se non gira nient'altro, aspetta la battuta. Gli ingressi 1 e 2 sono l'ingresso principale e alimentano ogni strand; gli ingressi da 3 a 10 sono una coppia stereo per strand, e uno strand registra la sua coppia sopra l'ingresso principale, così una coppia collegata gli dà una voce che gli altri non sentono. L'uscita principale porta gli ingressi in ascolto più ogni strand udibile, e le uscite da 3 a 10 sono uscite dirette stereo, una coppia per strand, dopo il suo Level, il mute e il solo. Play all e Stop all muovono tutti gli strand insieme. I loop si salvano con la patch come file audio nella sua cartella di progetto e si riaprono fermi, pronti per Rec o Play. Per mettere i loop sulla timeline, fai clic destro sull'organism e scegli Audio su traccia: uno strand, oppure Tutti per ogni strand registrato, ciascuno sulla sua traccia audio, alla battuta sotto la testina. Puoi anche trascinare fuori uno strand prendendolo dal centro del suo anello e lasciarlo sulla timeline dove vuoi. Collega un SoundIn all'ingresso e mappa il Rec1 di uno strand su un pedale per registrare, riprodurre e fare overdub con un piede solo.

L'anello di ogni strand disegna quello che c'è registrato: la forma d'onda del loop, luminosa dietro la testina e tenue davanti, con il numero di strati al centro. Un alone fuori dall'anello segue il livello in arrivo a quello strand, così vedi dove va il suono prima di armare qualcosa. Le due frecce sotto l'anello spostano di poco un loop in riproduzione: tieni premuta quella sinistra e lo strand va il tre per cento più lento, tieni quella destra e va il tre per cento più veloce, e quando lasci resta nel punto che ha raggiunto, così un tocco è uno spostamento minimo e una pressione lunga è un grosso spostamento. È il rimedio per un loop che sta un po' avanti o un po' indietro rispetto agli altri. Mentre tieni premuto il pitch scende o sale leggermente, come fa una mano su un disco.

## Parametri

**Rec1** Il flusso a un solo interruttore per un pedale o un pad, non presente sul pannello frontale: registra, chiudi e suona, overdub, suona. Tienilo premuto durante la riproduzione per fare overdub solo finché lo tieni. Lo stesso per gli strand da 2 a 4.

**Record1** Rec: avvia la registrazione su uno strand vuoto e, mentre registra, chiude il loop e lo fa suonare.

**Dub1** Dub: avvia un overdub su un loop in riproduzione o fermo e lo chiude alla pressione successiva; tienilo premuto per fare overdub solo finché lo tieni. Mentre registra chiude il loop e passa subito all'overdub.

**Play1** Fa ripartire lo strand dall'inizio del suo loop, sulla griglia quando Sync lo richiede. Lancia anche un loop che Stop ha chiuso muto, e mentre registra chiude il loop e lo fa suonare.

**Stop1** Ferma lo strand. Mentre registra chiude il loop muto, pronto da lanciare.

**Undo1** Toglie lo strato più recente. Mentre registra butta via il take.

**Redo1** Rimette l'ultimo strato tolto.

**Clear1** Svuota lo strand. Sul pannello frontale va tenuto premuto un attimo, così un clic accidentale non può farti perdere un loop.

**Level1** Il livello dello strand nel mix principale e sulla sua uscita diretta.

**Mute1** M: silenzia lo strand. Continua a girare sotto, quindi rientra in fase.

**Solo1** S: fa sentire solo questo strand.

**Sync1** Free, Beat, Bar o Follow: a cosa si agganciano le pressioni, Rev e Half. Follow usa il punto di giunzione dello strand col numero più basso che sta suonando, così il suo loop diventa la griglia; se svuoti quello strand, subentra il successivo in riproduzione. Un Helix nuovo parte con lo strand 1 su Bar e gli strand da 2 a 4 su Follow, così il primo loop fissa la lunghezza a cui si agganciano gli altri.

**Rev1** Suona lo strand al contrario. Commutarlo chiude un overdub aperto.

**Slice1** Quale quarto del loop dello strand è tenuto, 0 per nessuno. Clicca un quarto dell'anello per tenerlo e lascia per rilasciarlo, come i quattro pad di un controller da slicing; il parametro si mappa su un pedale o un pad come qualsiasi altro.

**Nudge1** -1 mentre tieni premuta la freccia sinistra, 1 mentre tieni quella destra, 0 altrimenti. Una spinta tenuta fa andare uno strand in riproduzione il tre per cento più lento o più veloce, e lo strand resta nel punto che ha raggiunto; non fa nulla a uno strand che sta registrando, facendo overdub o è fermo. Si mappa come qualsiasi altro parametro, così una coppia di pedali, o una rotella con ritorno al centro, sposta un loop senza mouse.


**Half1** Suona lo strand a metà velocità, un'ottava sotto. Uno strato registrato in overdub a metà velocità suona a velocità doppia quando lo disattivi.

**Shot1** Once: lo strand suona un solo passaggio e si ferma. Rec lo fa ripartire.

**Decay** Quanto degli strati più vecchi sopravvive a ogni passaggio di overdub. Sotto 1 un loop continua a evolversi invece di accumularsi.

**Monitor1** Off, Mix o Out: dove si sente l'ingresso live dello strand, cioè l'ingresso principale più la sua coppia. Mix lo fa suonare dall'uscita principale, e l'ingresso principale si sente una volta sola, per quanti strand scelgano Mix; Out lo fa suonare sull'uscita diretta dello strand, per uno strand che va su un suo canale. Lo stesso da Monitor2 a Monitor4.

**Monitor** L'unica impostazione di monitor delle patch più vecchie: uno strand senza un suo Monitor segue questa.

**Follow** Congela ogni strand, muto, mentre il transport è fermo e riparte quando riprende a scorrere, con gli strand sincronizzati di nuovo sulla battuta. Disattivato, i loop continuano a girare comunque. La registrazione non viene mai interrotta.

**PlayAll** Play all: fa ripartire ogni strand registrato dall'inizio del suo loop, tutti insieme, sulla griglia più larga che uno di loro usa: la battuta quando uno è su Bar, il beat quando uno è su Beat. Anche gli strand già in riproduzione ripartono, così i loop andati fuori fase si rimettono in fase. Uno strand impostato su Once che ha finito aspetta il suo Rec o Play.

**StopAll** Stop all: ferma ogni strand che sta girando, e uno strand ancora in registrazione chiude il suo loop muto. Aspetta la battuta solo quando ogni strand in movimento è su Bar, il beat quando nessuno è su Free; un solo strand su Free fa fermare tutto alla pressione.

**Loop1** Il file audio salvato dello strand, scritto accanto alla patch al salvataggio insieme al tempo a cui è stato registrato. Lo stesso da Loop2 a Loop4.

**Tempo1** Pitch, Stretch o Off: cosa fa lo strand quando il tempo si allontana da quello a cui è stato registrato. Pitch accelera o rallenta il loop come un nastro, pitch compreso. Stretch cambia solo la sua durata e mantiene il pitch, e un overdub mantiene il pitch a cui l'hai suonato; oltre otto volte il tempo di registrazione suona come Pitch. Off ignora il tempo. Lo stesso da Tempo2 a Tempo4.

## Ricetta

**Base agganciata, voce libera** Sync1 Bar, Sync2 Free, Follow transport attivo, ogni Monitor su Mix, Decay 0.85. Con il transport che scorre e un SoundIn collegato, premi Rec sul primo strand sull'uno, suona quattro battute e premi Dub sull'uno seguente per continuare ad aggiungere strati, poi di nuovo Dub per chiudere. Premi Rec sul secondo strand a metà frase per uno strand di voce che scivola rispetto alla griglia. Collega le uscite 3 e 4 a un Fern per avere solo il primo strand.

## Organism correlati

Repeater, Leafcutter, Sampler
