# PodIn

La via d'ingresso di un pod: un pin d'ingresso sul box del pod, portato a un'uscita all'interno.

Tutto quello che colleghi dall'esterno al box del pod passa per un PodIn e compare sulla sua uscita all'interno, un PodIn per pin, nell'ordine in cui sono stati creati. Aggiungine uno dal menu del clic destro sul canvas del pod o dalla categoria Pod, e collega la sua uscita a quello che, dentro il pod, deve sentirlo. L'editor della porta è un solo menu a tendina che la cambia di tipo sul posto tra Mono, Stereo, MIDI, Video e Controllo; la porta mantiene il nome, così ogni cavo sopravvive, e sul box del pod compare un pin corrispondente. Una porta di tipo Controllo è una presa sul box del pod: un Number, un LFO o un Follower collegato lì dall'esterno compare sull'uscita di controllo della porta, all'interno, pronto da trascinare su qualsiasi presa del pod. Una porta Video trasporta fotogrammi invece del suono e li passa avanti invariati.

## Ricetta

**Incapsula una catena di effetti** Crea un pod, aggiungi un PodIn Stereo e un PodOut Stereo, e collega il PodIn a un Fern e il Fern al PodOut. Aggiungi un PodIn di tipo Controllo e trascina la sua uscita sul Mix del Fern; fuori, il box del pod ora mostra un ingresso stereo, un'uscita stereo e una presa di controllo che un LFO può pilotare.

## Organism correlati

PodOut, Number, LFO
