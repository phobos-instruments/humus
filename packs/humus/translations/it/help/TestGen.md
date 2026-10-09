# TestGen

Un segnale di test prevedibile: una sinusoide a una frequenza, o rumore bianco, rosa o marrone.

Usalo per controllare un percorso del segnale, per impostare i livelli prima che ci sia musica, o per sentire da sola la forma di un filtro: una sinusoide mostra cosa fa un Filter a una frequenza, e il rumore contiene tutte le frequenze insieme e mostra cosa fa a tutte. Non ha ingressi e manda lo stesso segnale a entrambe le uscite, così un solo organism alimenta una catena stereo. Su un misuratore una sinusoide pura risulta molto più alta di come suona, quindi fidati di un VuMeter più che delle tue orecchie.

## Parametri

**Amplitude** Livello d'uscita. Parti basso; una sinusoide a fondo scala in una catena che non conosci è il modo in cui si danneggiano i monitor.

**Frequency** Altezza della sinusoide, da 10 Hz a 20 kHz. Viene ignorata mentre è selezionata una forma d'onda di rumore.

**Waveform** Sine è una frequenza e nient'altro. White noise ha la stessa energia per hertz, Pink noise la stessa energia per ottava, e Brown noise cala ancora più in fretta, per un rombo basso.

## Ricetta

**Controllo del filtro** Waveform White noise, Amplitude 0.3. Collegalo a un Filter, il Filter a uno Spectrum e poi a un SoundOut. Muovi Frequency del Filter e lo Spectrum disegna la curva che il filtro sta applicando.

## Organism correlati

Wave, Rhizome, VuMeter, Spectrum
