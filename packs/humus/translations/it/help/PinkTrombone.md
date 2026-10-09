# PinkTrombone

Una voce umana costruita su un modello della gola, basata su [Pink Trombone](https://dood.al/pinktrombone/) di Neil Thapen.

Non ci sono campioni né banchi di oscillatori: un modello della glottide produce il ronzio delle corde vocali, e una guida d'onda con la forma di gola, bocca e naso lo filtra in vocali, mormorii e sibili. Suonalo via MIDI: la nota tenuta è il pitch e la velocity spinge sul volume. La coppia Tongue guida la vocale, Squeeze stringe il tratto vocale fino a chiuderlo in Place per fricative e occlusive, e Nasal apre il passaggio verso il naso. Piccole oscillazioni di pitch e pressione tengono viva una nota tenuta. Collega un LFO o uno Slider a TongueX, Squeeze e Nasal e si mette a balbettare; collega la sua uscita a un Harmonizer o a un Trellis.

## Parametri

**TongueX** Front. Fa scorrere la gobba della lingua tra gola e denti; insieme a TongueY colloca la vocale.

**TongueY** Raise. Inarca la lingua verso il palato.

**Place** Dove viene stretto il tratto vocale: le labbra in cima alla manopola, la gola in fondo.

**Squeeze** Quanto viene stretto il tratto in Place. Niente per le vocali, un po' per le fricative, fino in fondo per bloccare l'aria, così che rilasciandola scoppi un'occlusiva.

**Nasal** Apre il velo palatino così il suono esce dal naso. Tienilo aperto per i mormorii di m e n, o colora una vocale con un pizzico di nasale.

**Tenseness** La pressione delle corde vocali, da un sussurro soffiato a un ronzio teso. La velocity scala il risultato.

**Vibrato** Profondità dell'ondulazione di pitch voluta e della lenta deriva che ci sta sotto.

**Wobble** Esagera la deriva involontaria fino a farne un lento ondeggiare della voce.

**Drone** Tiene la voce accesa anche senza note tenute, all'ultimo pitch ricevuto.

**Width** Allarga la voce sulle uscite stereo. A 0 i due lati portano la stessa voce; alzandolo, i lati si separano come un cantante in una stanza, e risommata in mono la voce resta invariata.

**Pan** Mette la voce a sinistra o a destra. Al centro passano entrambi i lati a pieno livello; fuori centro si abbassa solo il lato opposto.

**Level** Guadagno d'uscita.

**BendRange** Fin dove arriva la rotella del pitch a fine corsa, in semitoni. Due è il valore di default più comune; zero ignora la rotella.

## Ricetta

**Tappeto di coro** Drone attivo, Tenseness 0.5, Vibrato 0.3, TongueX 0.3 e TongueY 0.6 per una u. Manda una nota MIDI per fissare il pitch, collega un LFO a 0.1 Hz a TongueX con poca profondità perché la vocale vada alla deriva, e fai passare l'uscita per un Verbatim.

## Organism correlati

Harmonizer, Trellis, Decomposer
