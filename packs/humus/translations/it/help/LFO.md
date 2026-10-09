# LFO

Un oscillatore a bassa frequenza che emette un'onda come segnale di controllo per le manopole di altri organism.

Emette Offset più Amplitude per l'onda a Rate, sommato a quello che arriva al suo ingresso, così un LFO può oscillare attorno al valore di un Number. Con Sync attivo, un ciclo dura SyncBeats beat del transport invece di andare a Rate. Il pannello frontale disegna un ciclo dell'onda che sta emettendo, con una testina che la percorre. Fai clic destro su un parametro qualsiasi e Controlla con offre l'LFO due volte: wave è l'onda stessa e phase è la rampa ascendente del ciclo. Collega la sua uscita alla presa di un Gain per un tremolo o al Rate di un altro LFO per un movimento che cambia lentamente. Non porta audio.

## Parametri

**Rate** Cicli al secondo quando Sync è disattivato, da uno ogni cento secondi fino a 1000 Hz. La parte alta dell'escursione è a frequenza audio.

**Waveform** SIN, TRI, SQR, SAW, DSAW (un dente di sega discendente) o S&H, che salta a un nuovo valore casuale una volta per ciclo.

**Amplitude** Quanto si sposta l'onda da una parte e dall'altra di Offset.

**Offset** Il centro attorno a cui si muove l'onda. Alzalo per tenere positiva un'oscillazione bipolare.

**Sync** Aggancia un ciclo al transport invece che a Rate. Mentre il transport suona, il ciclo parte sul beat 1 e segue ogni salto, loop e scrub; mentre è fermo, l'onda continua a girare al tempo impostato.

**SyncBeats** Quanti beat dura un ciclo quando Sync è attivo. Un beat è un quarto, quindi 0.5 è un ottavo e 4 è una battuta di quattro quarti.

## Ricetta

**Tremolo sincronizzato** Sync attivo, SyncBeats 0.5, Waveform SIN, Amplitude 0.4, Offset 0.6. Collega l'uscita alla presa Gain di un Gain che porta un pad e il livello pulsa in ottavi; passa a SQR per un gate netto.

## Organism correlati

Number, Follower, Sig
