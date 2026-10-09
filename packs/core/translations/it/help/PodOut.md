# PodOut

La via d'uscita di un pod: un pin d'uscita sul box del pod, alimentato da un ingresso all'interno.

Tutto ciò che colleghi a un PodOut dentro il pod compare fuori, sul pin d'uscita corrispondente del box del pod: un PodOut per pin, nell'ordine in cui sono stati creati. Aggiungine uno dal menu del clic destro sul canvas del pod o dalla categoria Pod. L'editor della porta è un solo menu a tendina che la cambia di tipo sul posto tra Mono, Stereo, MIDI, Video e Controllo; la porta tiene il suo nome, così ogni cavo resta al suo posto, e sul box del pod spunta un pin corrispondente. Un PodOut di tipo Controllo è lo specchio del PodIn di tipo Controllo: tira un cavo da una qualsiasi uscita di controllo dentro il pod alla sua presa e sul box spunta un'uscita di controllo che porta il valore fuori, così un pod può essere una sorgente di controllo a sé. Una porta Video trasporta fotogrammi invece di suono e li passa oltre senza modificarli.

## Ricetta

**Pod come sorgente di controllo** Dentro un pod, collega un PodIn Stereo a un Follower e tira un cavo dall'uscita env del Follower a un PodOut di tipo Controllo. Fuori, collega la cassa al pod e tira un cavo dalla nuova uscita di controllo del pod a un Gain sul basso: il pod fa il ducking del basso in base a quello che sta ascoltando.

## Organism correlati

PodIn, Follower, Gain
