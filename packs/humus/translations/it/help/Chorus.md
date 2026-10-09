# Chorus

Un chorus stereo che ispessisce un suono con una copia ritardata che si muove lentamente.

Una copia dell'ingresso viene ritardata di pochi millisecondi e quel ritardo viene tenuto in movimento, così la copia sale e scende leggermente di intonazione attorno all'originale, come due musicisti che non cadono mai esattamente sullo stesso pitch. Lo sweep è un'onda triangolare, e quello del canale destro va in ritardo su quello del sinistro di RightLFOPhaseOffset: è questo che apre l'immagine. Rate e DelaySweepDepth interagiscono: la velocità con cui cambia il ritardo decide quanto varia il pitch, quindi raddoppiare l'uno o l'altro lo fa variare di più. Mettilo dopo un Rhizome, un Wave o un Sampler; un Flanger è la stessa idea con ritardi più brevi, un Phaser la stessa idea senza alcun ritardo.

## Parametri

**MinDelay** Il ritardo più breve che si raggiunge, in millisecondi. Valori piccoli scivolano verso il flanger, valori più grandi restano chiaramente chorus.

**DelaySweepDepth** Quanto il ritardo si sposta sopra MinDelay, in millisecondi. Più profondità vuol dire più detune.

**Rate** Quanto velocemente si sposta il ritardo, in Hz. Lento è un'onda morbida, veloce è un vibrato.

**RightLFOPhaseOffset** Di quanto lo sweep del canale destro va in ritardo su quello del sinistro, in gradi. A zero i due lati cambiano intonazione insieme e il risultato resta al centro; a 90 o 180 vanno ciascuno per conto suo e l'immagine si apre.

**Feedback** Rimanda la copia ritardata all'ingresso. Piccole quantità aggiungono risonanza; un valore negativo inverte il segnale a ogni giro e svuota il suono invece di ispessirlo.

**HFRolloffFrequency** Scurisce la copia ritardata dall'alto, in Hz. Tenere il percorso effettato più cupo di quello secco evita che un chorus suoni vitreo.

**Stereoness** Riservato alla larghezza dell'immagine effettata. Il chorus attuale tiene la copia di ciascun lato sul proprio lato, quindi questa manopola per ora non ha effetti udibili.

**WetDryMix** Quanto suono ritardato si sente rispetto all'originale. Tutto effettato, il detune non ha niente di stabile contro cui farsi sentire.

## Ricetta

**Pad largo** MinDelay 8, DelaySweepDepth 6, Rate 0.3, RightLFOPhaseOffset 180, Feedback 0, HFRolloffFrequency 8000, WetDryMix 0.5. Collega all'ingresso un accordo tenuto di Rhizome e i due lati si allontanano attorno al centro.

## Organism correlati

Flanger, Phaser, Fern, StereoTool
