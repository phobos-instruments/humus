# RNG

Una sorgente di numeri casuali estratti dall'entropia della macchina stessa, inviati come valore di controllo.

Ogni estrazione viene dall'entropia del sistema invece che da una formula, così la sequenza non si può ripetere né prevedere. L'uscita porta l'ultima estrazione, che viaggia su un cavo di controllo verso qualsiasi presa della patch, o dentro un Number per essere letta e passata avanti. Su una manopola, un'estrazione fatta su un certo intervallo vale come proporzione di quell'intervallo, quindi 200 su 255 è quasi in cima; su un Number si vede come il numero stesso. Il display in basso mostra l'estrazione, e la spia accanto a Rate lampeggia a ogni estrazione. Le estrazioni avvengono su un clock, sul beat del transport o a richiesta da Trigger.

## Parametri

**Rate** Estrazioni al secondo mentre Sync è disattivato, fino a 50. Zero ferma il clock, così estrae solo Trigger.

**Sync** Prende il clock delle estrazioni dal transport invece che da Rate, così le estrazioni cadono con la musica e seguono il tempo. Mentre il transport suona, un'estrazione cade sul beat 1 e poi a ogni confine di SyncBeats, ovunque tu salti nella timeline.

**SyncBeats** Tempo musicale tra un'estrazione e l'altra mentre Sync è attivo: un quarto di beat per qualcosa di irrequieto, quattro per un cambio a battuta.

**Format** Che tipo di numero è un'estrazione. 0 to 1 è il semplice valore unitario, Coin è quel valore arrotondato a zero o uno, e 8-bit, 8-bit signed, 16-bit e 16-bit signed estraggono un numero intero su quell'intervallo.

**Trigger** Estrae un numero su un fronte di salita, così un Button, una nota o un sensore possono chiedere un numero invece di aspettare il clock.

## Ricetta

**Nuova frequenza di taglio a ogni battuta** Sync attivo, SyncBeats 4, Format 0 to 1. Collega l'uscita alla presa Frequency di un Filter messo dopo un organism Drums e imposta l'intervallo del collegamento da 300 a 3000; la frequenza di taglio salta in un punto nuovo all'inizio di ogni battuta.

## Organism correlati

Number, Slider, LFO, Steps
