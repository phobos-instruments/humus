# Halogen

Un lettore spettrale che trasforma un'immagine in suono: da sinistra a destra è il tempo, dal basso in alto il pitch, la luminosità è il volume.

Una testina di scansione legge una colonna dell'immagine alla volta e una trasformata inversa la riporta in suono, così quello che sta sotto la testina è ciò che senti. Trascina un'immagine sul pannello o cliccalo per sfogliare; un file che non è un'immagine viene letto come byte grezzi disposti in righe, così anche un font o un archivio hanno uno spettro. Un cavo video da un VideoPlayer, un CameraIn o un VideoTrack sostituisce l'immagine con quella in movimento, fotogramma per fotogramma, e il file torna quando togli il cavo. Il pannello mostra l'immagine come la sente la testina, dopo Gate, Blur e Tilt. Collega l'uscita a un Fern o a un Verbatim per dare spazio.

## Parametri

**File** L'immagine che viene letta. Qualsiasi file: le immagini vengono decodificate, tutto il resto viene letto come byte.

**Scan** Sweep fa scorrere la testina lungo la finestra alla velocità di Rate. Free la parcheggia su X, così la muovono una manopola, un inviluppo o un trascinamento sul pannello.

**Reverse** Scorre nel verso opposto.

**Rate** Passate al secondo, da 0.01 a 100. Al massimo, lo scorrimento stesso diventa un tono.

**X** Dove sta la testina in Free, o dove comincia il tratto percorso.

**Y** Il fondo della banda letta.

**Width** Quanta parte della larghezza dell'immagine viene percorsa.

**Height** Quanta parte dell'altezza dell'immagine viene letta.

**Lowest** A che frequenza suona il fondo della banda, in hertz.

**Highest** A che frequenza suona la cima della banda, in hertz. Le righe sono distribuite in scala logaritmica tra i due.

**Gate** Una soglia di luminosità. I pixel sotto la soglia sono muti, così uno sfondo scuro non ronza.

**Blur** Ammorbidisce l'immagine, così una testina in movimento scivola sui bordi invece di andare a scatti.

**Tilt** Inclina il bilanciamento verso i bassi o verso gli acuti.

**Level** Livello d'uscita.

## Ricetta

**Deriva lenta** Carica una fotografia con lo sfondo scuro. Scan Sweep, Rate 0.1, Lowest 55, Highest 4000, Gate 0.15, Blur 0.4. Collega l'uscita a un Verbatim e lascia che una passata duri dieci secondi; poi metti Scan su Free e collega un LFO a X per muovere la testina.

## Organism correlati

Prism, Wave, SoundSpace
