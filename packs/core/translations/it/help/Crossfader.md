# Crossfader

Un crossfade stereo tra due sorgenti, con un solo controllo.

Gli ingressi 1-2 sono il lato A e gli ingressi 3-4 il lato B. Con Fade tutto in basso passa A, tutto in alto passa B, e in qualsiasi punto intermedio i due si mescolano secondo la legge scelta da Curve. Collega due organism Deck, due loop, oppure una copia secca e una processata dello stesso segnale, e porta l'uscita a un Mixer o a SoundOut. Pareggia i due lati con TrimA e TrimB prima di toccare Curve: un fade che va a scatti di solito vuol dire due sorgenti a livelli diversi, non la legge sbagliata. Fade può anche pilotare altri parametri tramite Controllo parametri, così un solo movimento può aprire un filtro mentre attraversa.

## Parametri

**MasterGain** Livello d'uscita dopo il fade e i trim.

**TrimA** Livello del lato A prima del fade, per pareggiare prima le due sorgenti.

**TrimB** Lo stesso per il lato B.

**Fade** La posizione del crossfade. In basso è A, in alto è B.

**Curve** La legge del fade. Zero è una linea retta, giusta per una copia secca e una processata dello stesso suono; circa due terzi è a potenza costante, giusta per due sorgenti non correlate; uno è un taglio rapido, dove un piccolo movimento vicino a uno dei due estremi cambia il suono di colpo.

**CutA** Finché lo tieni premuto, il mix salta sul lato A qualunque cosa dica Fade, e torna a Fade quando lo rilasci. Tenendo premuti A e B insieme il fade si ferma a metà.

**CutB** Lo stesso per il lato B.

**CutTime** Quanto ci mette un taglio ad arrivare, in millisecondi. Zero scatta subito; pochi millisecondi tolgono il clic da un basso tenuto.

## Ricetta

**Mix a due deck** Collega un Deck agli ingressi 1-2 e un altro ai 3-4, imposta Curve a circa 0.67 e regola TrimA e TrimB finché ogni deck dà la stessa lettura sul misuratore del Mixer con Fade a ciascun estremo. Mappa CutA e CutB su due pad e imposta CutTime a circa 5 per tagliare il brano in entrata sopra quello in uscita senza clic.

## Organism correlati

Mixer, Gain, Console, Send
