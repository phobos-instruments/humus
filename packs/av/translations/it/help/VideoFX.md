# VideoFX

Un trattamento dell'immagine con controlli di posizione, scala, rotazione, colore e mosaico.

Un ingresso video e un'uscita. L'immagine passa per uno stadio di trasformazione (posizione, scala, rotazione, specchio e pixelizzazione) e poi per uno stadio del colore (luminosità, contrasto, saturazione, tinta e inversione). Ogni manopola è un parametro come gli altri, quindi un Follower su Scale spinge l'immagine con la cassa, un LFO su Rotate la fa girare piano e un Sequence su Hue fa ruotare i colori a tempo. Mettilo tra un VideoPlayer, un VideoPad o un CameraIn e un Lumen, un VideoMix o un VideoOut, e concatenane due quando trasformazione e colore devono avere automazioni separate.

## Parametri

**PosX** La posizione orizzontale dell'immagine, in proporzione al fotogramma. +100% la sposta di un fotogramma intero verso destra.

**PosY** Lo stesso, in verticale. +100% la sposta di un fotogramma intero verso l'alto.

**Scale** Quanto è grande. 100% è com'era, 200% grande il doppio, 0% la riduce a un punto.

**Rotate** La rotazione in gradi attorno al centro del fotogramma.

**Pixelate** Quanto è grosso il mosaico. 0% lascia stare l'immagine; 100% è una manciata di blocchi in larghezza.

**Mirror** Ripiega il fotogramma a metà. Off lo lascia intero, Left copia la metà sinistra su quella destra, Top copia la metà superiore su quella inferiore, Both fa entrambe le cose.

**Brightness** La luce. 100% è com'era, 0% è nero, 200% luminoso il doppio.

**Contrast** L'escursione attorno al grigio medio. 0% appiattisce tutto sul grigio, 200% spinge verso gli estremi.

**Saturation** La manopola Colour. 0% è monocromo, 100% com'era, 200% sovrasaturo.

**Hue** Fa ruotare ogni colore lungo la ruota cromatica, in gradi.

**Invert** Mostra il negativo.

## Ricetta

**Zoom sulla cassa** Collega un VideoPlayer a VideoFX e VideoFX a un VideoOut. Collega la cassa a un Follower con un rilascio breve e manda il Follower su Scale con poca profondità sopra il 100%, così ogni colpo spinge l'immagine verso di te e tra un colpo e l'altro torna a posto. Aggiungi un tocco di Pixelate per un bordo ruvido.

## Organism correlati

VideoPlayer, VideoPad, VideoMix, Lumen
