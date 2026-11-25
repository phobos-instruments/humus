# OscMonitor

Un registro dei messaggi OSC che attraversano l'app, in entrambe le direzioni, come righe leggibili.

Non ha ingressi né uscite: mettine uno dove vuoi ed elenca ogni messaggio che arriva sulla porta d'ingresso OSC, mappato a un parametro o no, e ogni valore che trasmette un organism che invia OSC, come Hands, con l'indirizzo a cui è andato. Usalo per verificare che un controller sul telefono o un ponte per sensori raggiunga davvero la macchina, per trovare l'indirizzo che manda un fader prima di mapparlo, e per confermare cosa deve aspettarsi un ricevitore. Un pacchetto che non è affatto OSC compare come (non OSC), con la sua dimensione e l'indirizzo da cui è arrivato, così un dispositivo che parla un'altra lingua si riconosce subito. L'ingresso OSC deve essere attivo in Impostazioni perché compaiano le righe ricevute; l'intestazione dice quando è disattivato. Pause congela la lista e Clear la svuota.

## Parametri

**Direction** Mostra le righe All, Received o Sent. Una mano tracciata invia di continuo, quindi Sent può inondare la vista; le righe filtrate vengono scartate, non conservate.

## Ricetta

**Trovare un indirizzo** Imposta Direction su Received, punta il controller sulla porta OSC della macchina e muovi un fader. L'indirizzo che compare è quello da dare alla mappatura OSC del parametro.

## Organism correlati

Hands, MidiMonitor
