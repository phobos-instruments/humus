# Number

Un box numerico che mostra un valore dal vivo, lo riceve da un ingresso di controllo e lo inoltra da un'uscita di controllo.

I cavi portano suono e i numeri arrivano sulle manopole; un Number è dove guardi un numero mentre va verso una manopola. Collega al suo ingresso un Follower, uno Slider, un RNG o un altro Number e il box mostra quello che arriva, senza scalarlo; scrivi un valore e resta lì finché non arriva qualcosa di nuovo. Qualsiasi manopola della patch può seguire l'uscita: traccia un cavo di controllo su una presa, oppure fai clic destro sulla manopola e scegli Controlla con. Siccome Value è un parametro come gli altri, anche le corsie di automazione e il controllo MIDI possono pilotarlo, e questo fa di Number il ponte tra loro e ogni presa della patch.

## Parametri

**Value** Il numero che il box contiene. Lo imposta l'ingresso, lo imposti tu scrivendolo, e l'uscita lo inoltra.

**Format** La forma con cui il numero esce. Any invia esattamente quello che contiene, Whole arrotonda a un intero, e 8-bit, 8-bit signed, 16-bit e 16-bit signed lo arrotondano e lo limitano a quella larghezza, così il box mostra quello che esce davvero.

## Ricetta

**Sidechain a vista** Collega l'uscita di un Follower all'ingresso di un Number e l'uscita del Number alla presa Gain di un Gain sul basso. Il box legge l'inviluppo mentre si muove, così vedi il livello che raggiunge il ducking prima di impostare l'intervallo del collegamento.

## Organism correlati

Slider, Sig, Follower, Gain
