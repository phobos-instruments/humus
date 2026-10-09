# Slider

Un fader la cui posizione è un valore di controllo che qualsiasi manopola della patch può seguire.

L'uscita porta la posizione del cursore come valore di controllo, così una manopola può seguirla tramite un cavo di controllo o un collegamento Controlla con, e l'ingresso imposta il cursore da un altro controllo, così un sensore o un LFO lo possono muovere. Min e Max sono i numeri che rappresentano i due estremi e l'uscita dichiara quell'intervallo, così una manopola segue la posizione lungo la sua corsa, mentre un Number collegato alla stessa uscita legge il valore mappato. Mappa un fader hardware su Value con il MIDI learn e diventa una macro che guida più collegamenti insieme. Dai a un collegamento una forma a interruttore e lo Slider fa scattare un Bypass o apre un Gate oltre una soglia.

## Parametri

**Value** Il cursore, da 0 a 1. L'ingresso lo imposta e l'uscita lo invia.

**Min** Il numero che rappresenta il fondo della corsa. Con Min sopra Max la corsa si inverte.

**Max** Il numero che rappresenta la cima della corsa.

**Slew** Quanto tempo impiega il valore a raggiungere una nuova impostazione, in millisecondi. A 0 arriva subito; più alto trasforma un salto in una scivolata.

**Log** Curva la corsa tra Min e Max in modo logaritmico, così la parte bassa ha più spazio. Usalo per frequenze e tempi; entrambi gli estremi devono essere sopra zero.

## Ricetta

**Macro del filtro** Min 80, Max 12000, Log attivo, Slew 60. Fai clic destro su Cutoff di un Filter e scegli Controlla con Slider, poi fai clic destro su Feedback di un Fern e fai lo stesso con un intervallo più stretto. Ora un solo fader apre il filtro e allunga l'eco insieme; mappa Value su un fader hardware con il MIDI learn.

## Organism correlati

Number, Sig, LFO, Gate
