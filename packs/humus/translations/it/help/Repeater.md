# Repeater

Un loop roll sull'audio dal vivo: tieni premuto Repeat e l'ultimo pezzo di suono si ripete sulla griglia del transport.

Il Repeater ascolta sempre. Premi Repeat e l'ultimo tratto di audio lungo Length, da un sedicesimo di battuta a quattro battute, suona in ripetizioni che cadono sul beat; rilascia e torna il segnale dal vivo. Di default ogni ciclo riafferra la finestra più recente, così i movimenti di filtro e i cambi di pattern continuano a scorrere nelle ripetizioni; Hold invece blocca una slice e la ripete finché non molli, anche se la sorgente si ferma. I bordi del loop hanno un crossfade, così anche il materiale melodico va in roll senza clic, e con il transport fermo ripete comunque l'ultimo tratto lungo Length che finisce adesso. Fai passare attraverso il Repeater un organism Drums o un Deck.

## Parametri

**Repeat** Avvia il roll finché è premuto. Mappalo su un pad o un tasto per suonarlo tenendo premuto.

**Hold** Blocca la slice catturata quando hai premuto Repeat, invece di seguire il segnale dal vivo. Il freeze per i build-up.

**Length** La dimensione della finestra, da 1/16 di battuta a 4 battute. Cambiala durante il roll e il roll si ritaglia sulla stessa griglia.

**Mix** A 1 il roll sostituisce il segnale; abbassalo per sovrapporre il roll al materiale dal vivo.

## Ricetta

**Build che stringe** Collegaci un organism Drums, Mix 1, Hold disattivato. Mappa Repeat su un pad e, tenendolo premuto, porta Length da 1 battuta a 1/2 battuta, poi 1/4 di battuta e infine 1/8 di battuta nelle ultime battute prima di un drop, poi rilascia sul battere.

## Organism correlati

Deck, Riff, Steps
