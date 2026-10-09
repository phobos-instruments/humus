# SoundSpace

Un granulatore che si suona facendo volare un cursore su una mappa di grani ordinati per suono, e che registra anche quello che gli arriva.

Carica fino a quattro file audio; SoundSpace li taglia in grani, analizza di ognuno energia, brillantezza, rumorosità e movimento, e li sparge su una mappa bidimensionale dove i grani simili finiscono vicini, un colore per file. Il cursore suona una nuvola dei grani intorno a sé, così trascinarlo sulla mappa fa un morphing tra le texture. Mescola materiale molto diverso, voce, macchine e field recording, e la mappa si allarga, il che rende il viaggio più spettacolare. Fai clic destro sulla mappa per automatizzare X e Y o farne il MIDI learn, così un pad XY o un LFO lento possono guidarla. Live è attivo fin dall'inizio, quindi l'ingresso è già una sorgente a sé e basta collegarci qualcosa: i suoi ultimi otto secondi vengono tagliati e lasciati cadere sulla stessa mappa man mano che arrivano, punti chiari tra i colori dei file, i più recenti più grandi, così vedi dove cade quello che suoni e puoi far volare il cursore tra quello e le registrazioni. Il misuratore accanto a Freeze mostra il livello dell'ingresso, quindi una mappa vuota con il misuratore spento vuol dire che non sta arrivando niente. Le due cose convivono: suona nell'ingresso mentre ci sono file caricati e la mappa li tiene entrambi, e Rec cattura l'ingresso nel primo slot di file vuoto, così un passaggio che ti è piaciuto diventa uno dei file. Live non ha bisogno di file: senza file caricati, la mappa va dallo scuro al brillante in orizzontale e dall'intonato al rumoroso verso l'alto.

## Parametri

**Mute** Silenzia l'uscita mentre la nuvola continua a volare, così togliendo il mute rientri a metà texture.

**Record** Il pad rosso Rec. Cattura l'ingresso audio in un file audio sotto Documenti e lo mette nel primo slot vuoto quando si ferma. I take silenziosi vengono scartati, e un take dura al massimo due minuti.

**Live** Granula l'ingresso audio mentre arriva, ed è attivo fin dall'inizio. Gli ultimi otto secondi restano sulla mappa; il suono più vecchio sparisce, e il silenzio viene lasciato fuori. Disattivalo per suonare solo i file caricati.

**Freeze** Tiene fermo il buffer live, così la nuvola continua a suonare quello che ha catturato mentre l'ingresso viene ignorato. Solo con Live attivo.

**X** La posizione orizzontale del cursore sulla mappa, da 0 a 1.

**Y** La posizione verticale del cursore sulla mappa, da 0 a 1.

**Density** Grani al secondo.

**Scatter** Quanto irregolarmente arrivano i grani. A zero arrivano esattamente al ritmo di Density, come un orologio; alzandolo, ogni intervallo viene estratto tra quasi niente e quasi il doppio, mentre la frequenza media resta dove la mette Density, così la nuvola inciampa e si raggruppa invece di ticchettare.

**GrainSize** Lunghezza del grano in millisecondi. Apri l'intervallo e ogni grano prende la sua lunghezza al suo interno.

**Fit** Adatta il pad a dove si trovano davvero i suoni. Un banco i cui punti finiscono in un angolo riempie il pad invece di nascondersi lì dentro, e i numeri lungo i bordi si riferiscono alla finestra invece che all'intero spazio. Non ingrandisce mai oltre un quarto dello spazio, così un singolo gruppo stretto non diventa un microscopio, e si riadatta solo quando cambiano i punti - mai sotto la tua mano mentre trascini. Disattivato dà l'intero spazio, come prima.

**Spray** Raggio di scelta intorno al cursore. Piccolo resta su una texture; grande pesca da tutto il vicinato.

**Spread** Di quanto ogni grano viene spostato a caso nel panorama, lontano dal centro.

**Pitch** Trasposizione in semitoni. Chiuso è una sola trasposizione; aprilo e ogni grano viene estratto da un punto dentro l'intervallo, così un intervallo solo verso l'alto crea uno shimmer che sale invece di una sfocatura simmetrica.

**Skew** Dove cade il picco dentro un grano, con un intervallo come gli altri. Chiuso a zero è una campana morbida; verso uno la salita si accorcia finché il grano diventa un colpo che risuona, e questo trasforma un tappeto in un ritmo anche alla stessa densità.

**Shape** Quanto di un grano resta aperto a pieno livello tra la salita e la discesa. A zero il grano è tutto salita e discesa; verso uno diventa un blocco col tetto piatto, che dallo stesso materiale tira fuori una texture più morbida e continua.

**Input** Livello dell'ingresso quando entra nel buffer live, fino a una volta e mezza. Abbassalo per lasciare che prenda il sopravvento Feedback, alzalo per tenere in primo piano quello che suoni di nuovo. Solo con Live attivo.

**Feedback** Quanto della nuvola viene riscritto nel buffer live. Al massimo la nuvola si alimenta da sola per sempre e non si spegne mai; la riscrittura ha un limitatore morbido, quindi si ispessisce e satura invece di andare fuori controllo. Con Pitch lontano da zero ogni passaggio cade un intervallo più su o più giù, così una nota tenuta si impila in un accordo. Solo con Live attivo, e non mentre è congelato.

**Mix** Bilanciamento tra l'ingresso secco e la nuvola. Di default è tutto effettato.

**Quant** Quanto forte un grano viene tirato sulla griglia scelta da Sync. A zero non viene tirato per niente, al massimo aspetta la linea. Non fa niente mentre Sync è su Free.

**Decorrelation** Quanto le scelte casuali di un grano vanno d'accordo tra loro. A zero un solo tiro decide insieme pitch, lunghezza, tenuta e panning, così i grani più acuti finiscono da un lato; al massimo ognuno viene estratto per conto suo.

**Voices** Il massimo di grani che possono suonare insieme, da 1 a 200. Meno assottiglia la texture e costa meno; più la ispessisce.

**Sync** Un selettore rotativo che aggancia ogni grano alla griglia del transport: Free, da un trentaduesimo a un quarto, o una battuta. Density decide ancora quanto spesso tocca a un grano; Sync lo fa aspettare fino alla linea successiva. Solo mentre il transport suona.

**Gain** Livello d'uscita.

**File1** Il primo file del corpus, e così via da File2 a File4. L'analisi parte quando scegli un file.

## Ricetta

**Tappeto in evoluzione** Carica una registrazione di voce in File1 e un field recording in File2. Density 12, GrainSize 200, Spray 0.08, Spread 0.8, Pitch aperto un po' da entrambi i lati. Collega un LFO a 0.02 Hz su X e un altro a 0.03 Hz su Y, così il cursore vaga per la mappa, e manda l'uscita in un Verbatim.

**Nuvola dal vivo** Nessun file, Live attivo, una voce o uno strumento collegati all'ingresso. Density 20, GrainSize 150, Spray 0.3, Pitch aperto appena. Suona una frase, premi Freeze, poi fai volare il cursore attraverso quello che hai appena suonato. Per gli accordi, togli il freeze, imposta Pitch su 7 e porta Feedback a 0.6.

## Organism correlati

CameraIn, Sampler
