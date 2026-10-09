# VideoMix

Un crossfader A/B per due flussi video, o un mixer a canali per un massimo di otto.

Inputs decide quanti flussi riceve. A 2 è un crossfader: l'ingresso 1 è A, l'ingresso 2 è B, e l'uscita porta la dissolvenza. Fade a 0 mostra A, a 1 mostra B, e in qualsiasi punto in mezzo mescola i due secondo la curva impostata da Curve. Fade è un parametro come gli altri: mappaci un controller per avere un crossfader hardware, mettici un LFO sincronizzato così i due flussi si scambiano di posto ogni sedici beat, o disegnalo in una corsia di automazione così la sequenza dei tagli fa parte del brano. Fai il crossfade tra due composizioni complete di Lumen prima del VideoOut, o scegli più ingressi per un mixer a canali.

Da 3 ingressi in su diventa un mixer a canali con un fader verticale per ogni ingresso. I canali si impilano in ordine, il canale 1 in basso, e ogni fader dice quanto quel canale copre quelli sotto: tutto su li nasconde, a metà li lascia intravedere, tutto giù sparisce. Il canale 1 parte alzato e gli altri abbassati, così alzando un fader porti il suo flusso sopra l'immagine. Quando cambi il numero di ingressi i cavi restano dove sono; un cavo su un ingresso che la nuova dimensione non ha passa a uno libero, oppure viene tolto.

## Parametri

**Inputs** Quanti flussi, da 2 a 8. Con due è il crossfader descritto sotto; con di più è il mixer a canali.

**Level 1 to 8** I fader del mixer a canali, uno per ingresso: quanto quel canale copre i canali che ha sotto.

**Fade** La dissolvenza. 0 è tutto A (ingresso 1), 1 è tutto B (ingresso 2), e in mezzo c'è un mix modellato da Curve.

**Curve** La curva di dissolvenza del crossfader, la stessa manopola di Crossfader. A 0 è lineare, quindi a metà strada entrambe le immagini sono a metà luminosità e il fotogramma si scurisce. Più alta tiene luminoso ciascun lato più a lungo durante il fade; due terzi è la curva a potenza costante, e 1 è la versione più netta.

## Ricetta

**Scambio a tempo di battuta** Collega un VideoPlayer all'ingresso 1, un CameraIn all'ingresso 2 e l'uscita a un VideoOut. Imposta Curve a due terzi e aggiungi un LFO, poi fai clic destro su Fade, scegli Controlla con e l'LFO, e sincronizza l'LFO su 8 beat con un'onda quadra, così i flussi si scambiano sulla battuta.

**Quattro flussi, uno schermo** Imposta Inputs a 4 e collega un CameraIn, due VideoPlayer e un VideoPad agli ingressi da 1 a 4. Lascia alzato il canale 1 come base, poi mappa i fader da 2 a 4 su tre fader hardware e alzali uno alla volta per far entrare il loro flusso sopra gli altri.

## Organism correlati

Lumen, VideoPlayer, VideoOut
