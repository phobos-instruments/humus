# Spark

Un riser che non arriva mai: dodici brevi linee di ritardo percorrono come filtri a pettine le stesse ottave, senza mai uscirne.

Sei linee per canale, ognuna lunga solo pochi millisecondi e ognuna rimescolata al segnale secco, così risuona come un pettine. Ogni linea porta il suo pettine lungo la stessa estensione di ottave, ma ognuna parte da un punto diverso, entra sfumando in basso ed esce sfumando in alto sotto la propria finestra di guadagno; sei finestre distribuite in modo uniforme sommano a una costante, quindi non si sente nessuna giuntura e il pitch sale per sempre. Time e Range fissano dove vive il pettine, da un fischio con tempi brevi a un corpo che sfarfalla con tempi lunghi. Randomizza estrae a caso Rate, Time, Range, Feedback, Damp, Spark e Spread; Mode, Mix e Level restano dove sono. Fai passare attraverso Spark un pad, un organism Drums o un intero bus di un Mixer.

## Parametri

**Mode** Rise sale, Fall scende, e Free va avanti e indietro lungo l'estensione invece di ricominciare da capo.

**Rate** Quanto velocemente una linea attraversa tutta l'estensione, in Hz. Il valore predefinito impiega circa otto secondi per ogni sweep.

**Time** Il ritardo più corto che una linea raggiunge, in millisecondi: la cima della sua salita.

**Range** Quante ottave sotto Time parte la linea.

**Feedback** Quanto a lungo risuona ogni pettine. Basso colora il suono e passa oltre; alto trasforma ogni linea in un dente stretto e risonante, con un pitch tutto suo.

**Damp** Smussa gli acuti dentro il loop di feedback, così le code lunghe diventano calde invece che stridule.

**Spark** Raggruppa la spaziatura delle linee. A zero le linee sono distribuite in modo uniforme e la salita è morbida; più in alto, le linee finiscono una sopra l'altra e i loro denti si rinforzano. Gli scarti sono fissi, quindi una regolazione dà sempre lo stesso risultato.

**Spread** Sfasa il canale destro rispetto al sinistro, così i due lati salgono fuori passo e l'immagine si allarga.

**Mix** Secco contro filtrato a pettine. Il pettine ha bisogno di entrambe le metà, quindi è nel mezzo che vive.

**Level** Livello d'uscita.

## Ricetta

**Salita senza fine** Mode Rise, Rate 0.06, Time 2, Range 4, Feedback 0.8, Damp 0.4, Spark 0.2, Spread 0.6, Mix 0.5. Collega in ingresso un pad tenuto e lascialo andare sotto una sezione; passa Mode su Fall per il rilascio dopo il drop.

## Organism correlati

Fern, Flanger, Chorus
