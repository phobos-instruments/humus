# SideChain

Un compressore la cui riduzione di guadagno è guidata da un ingresso key separato.

Il segnale Main sugli ingressi 1-2 viene abbassato dal livello del Key sugli ingressi 3-4: collega la cassa al Key e il basso o il pad al Main, e il Main si abbassa a tempo con la cassa. Il Key serve solo al rilevatore e non passa mai in uscita, quindi collega la cassa anche al tuo Mixer per conto suo. Il menu a tendina Mode passa tra stereo e mono sul posto; in mono, l'ingresso 1 è Main e l'ingresso 2 è Key. Se non c'è niente sul Key, il Main passa intatto. Per un ducking identico a ogni colpo, indipendentemente dalla cassa, usa invece SideKick.

## Parametri

**Detector** RMS fa la media del Key, così la riduzione aggancia in ritardo e molla lentamente, il classico carattere pumping. Peak segue esattamente ogni transiente.

**Threshold** Il livello del Key sopra il quale inizia la riduzione, in dB. Impostalo in modo che la cassa lo superi chiaramente.

**Ratio** Quanto viene abbassato il Main quando il Key supera la soglia. Da 4 a 8 dà un pumping evidente.

**Knee** Quanto gradualmente entra la riduzione attorno alla soglia, in dB.

**Attack** Quanto velocemente entra la riduzione, in millisecondi. Tienilo a pochi millisecondi così passa il transiente della cassa.

**Hold** Quanto a lungo la riduzione resta alla massima profondità dopo che il Key scende, in millisecondi.

**Release** Quanto velocemente si riprende il Main, in millisecondi. Da 150 a 250 ms lo lascia risalire prima della cassa successiva.

**Makeup** Guadagno aggiunto dopo la compressione, in dB.

**Mix** Sotto 1 miscela il segnale in ducking con quello secco, per un ducking in parallelo.

**Listen** Manda il Key alle uscite al posto del Main, così puoi impostare Threshold a orecchio. Poi ricordati di disattivarlo.

## Ricetta

**Basso che pompa** Collega il Kick agli ingressi 3-4 e la linea di basso agli ingressi 1-2. Detector RMS, Threshold -20, Ratio 6, Attack 2, Release 180, Mix 1. Collega il Kick anche al Mixer con un suo cavo, poi muovi Release finché il basso non risale appena prima della cassa successiva.

## Organism correlati

SideKick, Kick, Microdot
