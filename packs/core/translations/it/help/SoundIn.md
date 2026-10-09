# SoundIn

Un segnale stereo dal vivo dall'interfaccia audio, o un file audio suonato al suo posto.

Mettine uno nella patch, scegli la coppia di ingressi, e un microfono, uno strumento o un segnale di linea entrano dritti nella patch; collega le sue uscite a un Gain, a un Fern o al Mixer. Il menu a tendina Input elenca i nomi degli ingressi dell'interfaccia stessa, e il misuratore mostra cosa arriva prima di Gain. Con Live input disattivato suona invece il file audio, ed è anche così che una patch si renderizza offline: se non c'è un ingresso dal vivo disponibile, si usa il file. Un'interfaccia mono riempie entrambe le uscite con il suo unico ingresso. Metti più SoundIn puntati su coppie diverse per trattare separatamente ogni coppia di un'interfaccia multicanale. Le patch più vecchie con ingressi ausiliari numerati si caricano ancora, e ognuno porta il suo canale come parametro.

## Parametri

**Channel** Quale coppia di ingressi del dispositivo legge questo SoundIn, mostrato come Input; 1/2 è la coppia principale. Il cambio vale subito, dal vivo.

**UseADC** Live input. Attivo porta l'ingresso dell'interfaccia; disattivato suona invece il File qui sotto.

**File** Il file audio usato mentre Live input è disattivato. Si carica appena lo imposti, e un file mono riempie entrambe le uscite.

**Loop** Ripete il file quando arriva alla fine. Disattivato, il file suona una volta e poi le uscite tacciono.

**Gain** Una regolazione pulita del livello d'ingresso, a guadagno unitario di default. Abbassalo per domare un ingresso troppo forte, alzalo per sollevare una sorgente debole; il misuratore legge il livello prima di questo controllo.

## Ricetta

**Microfono nella patch** Imposta Input sulla coppia a cui è collegato il microfono, lascia Live input attivo e collega le uscite a un Gain e poi a un Fern prima del Mixer. Regola Gain in modo che il misuratore stia ben sotto il massimo sulla tua frase più forte, poi carica un File e disattiva Live input per continuare a lavorare sulla patch senza microfono.

## Organism correlati

SoundOut, FilePlayer
