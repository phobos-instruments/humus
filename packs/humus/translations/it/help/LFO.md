# LFO

Un oscillatore a bassa frequenza che emette un'onda come segnale di controllo per le manopole di altri organism.

Emette Offset più Amplitude per l'onda a Rate, sommato a quello che arriva al suo ingresso, così un LFO può oscillare attorno al valore di un Number. Con Sync attivo, un ciclo dura SyncBeats beat del transport invece di andare a Rate. FN suona una forma tua: collega un Math alla presa Function e l'LFO suona la formula del Math come sua onda, oppure collega un Wave e suona la tabella del Wave. Il pannello frontale disegna un ciclo dell'onda che sta emettendo, con una testina che la percorre, oppure su FN il suo moto recente. Fai clic destro su un parametro qualsiasi e Controlla con offre l'LFO due volte: wave è l'onda stessa e phase è la rampa ascendente del ciclo. Collega la sua uscita alla presa di un Gain per un tremolo o al Rate di un altro LFO per un movimento che cambia lentamente. Non porta audio.

## Parametri

**Rate** Cicli al secondo quando Sync è disattivato, da uno ogni cento secondi fino a 1000 Hz. La parte alta dell'escursione è a frequenza audio. Su FN è anche rate nella formula, così un moto che avanza di rate*dt va al suo passo.

**Waveform** SIN, TRI, SQR, SAW, DSAW (un dente di sega discendente), S&H, che salta a un nuovo valore casuale una volta per ciclo, oppure FN, la formula o la tabella passata attraverso Function.

**Amplitude** Quanto si sposta l'onda da una parte e dall'altra di Offset.

**Offset** Il centro attorno a cui si muove l'onda. Alzalo per tenere positiva un'oscillazione bipolare.

**Sync** Aggancia un ciclo al transport invece che a Rate. Mentre il transport suona, il ciclo parte sul beat 1 e segue ogni salto, loop e scrub; mentre è fermo, l'onda continua a girare al tempo impostato.

**SyncBeats** Quanti beat dura un ciclo quando Sync è attivo. Un beat è un quarto, quindi 0.5 è un ottavo e 4 è una battuta di quattro quarti.

**Function** Collega qui un Math per passare la sua formula all'LFO. Il cavo porta la formula e le manopole X, Y, Z e W del Math invece di un valore: l'LFO passa a FN, segue ogni modifica finché il cavo c'è e tiene quello che ha quando lo stacchi. Nella formula, phase è il ciclo dell'LFO e rate il suo Rate, così i preset Nature di un Math si muovono al passo dell'LFO. Un Wave collegato passa invece la sua tabella: l'LFO suona i frame alla Position del Wave e segue quella manopola.

## Ricetta

**Tremolo sincronizzato** Sync attivo, SyncBeats 0.5, Waveform SIN, Amplitude 0.4, Offset 0.6. Collega l'uscita alla presa Gain di un Gain che porta un pad e il livello pulsa in ottavi; passa a SQR per un gate netto.

**Palla che rimbalza** Carica il preset Nature Gravity in un Math, collega il Math alla presa Function dell'LFO e l'uscita dell'LFO al cutoff di un filtro. Il timbro segue una palla che rimbalza sempre prima e sempre più bassa finché si ferma, poi viene rilanciata; Rate accelera la palla e la X del Math la rende più vivace.

**Onda che vaga** Scrivi `walk := walk*(1 - rate*dt) + sqrt(6*rate*dt)*noise(); sin(tau*ph(rate*(1 + 0.15*x*walk)))` in un Math e collegalo a Function: un seno al Rate dell'LFO la cui velocità deriva come una mano sulla manopola, che non si ripete mai del tutto.

## Organism correlati

Math, Number, Follower, Sig
