# Filter

Un filtro a variabili di stato con uno stadio a valvole, nella tradizione del filtro da rack per club.

Uno stadio di drive alimenta un filtro risonante i cui pulsanti HI, BAND e LO si sommano invece di escludersi: HI con LO è un notch da far scorrere per un effetto phaser, tutti e tre insieme sono un passa-tutto che Resonance rende drammatico, e con nessuno selezionato il segnale passa pulito. Un inseguitore di inviluppo e uno sweep a onda triangolare o quadra possono spingere entrambi la frequenza di taglio, oppure l'inviluppo può andare invece al drive. Ogni commutazione è morbida, così puoi usare i pulsanti dal vivo senza clic. Mettilo dopo un Rhizome, un organism Drums o un intero Mixer, e collega un LFO o un Follower a Frequency per avere più forme di quelle degli sweep interni.

## Parametri

**EnvFollow** Quanto il volume dell'ingresso apre la frequenza di taglio. Zero lascia il filtro fermo.

**EnvDecay** Rilascio Fast o Slow per l'inviluppo quando l'ingresso cala.

**Drive** Lo stadio a valvole. Man mano che lo spingi aggiunge prima calore, poi soft clipping, poi hard clipping, con un guadagno di compensazione così il livello resta fermo.

**EnvToDrive** Manda l'inviluppo a Drive invece che alla frequenza di taglio, così i passaggi forti distorcono di più.

**LfoSpeed** Velocità dello sweep da 0.2 Hz a 10 kHz. Oltre qualche centinaio di hertz lo sweep stesso diventa un tono.

**LfoWave** Triangle scivola tra gli estremi dello sweep. Square salta dall'uno all'altro.

**LfoSync** Aggancia lo sweep al transport invece che a LfoSpeed.

**LfoBeats** La durata di uno sweep sincronizzato in beat, così 4 è una battuta di quattro quarti.

**LfoDepth** Quanto lo sweep spinge la frequenza di taglio, fino a qualche ottava in ciascuna direzione.

**Resonance** Da un'attenuazione morbida a un picco da synth. Si riduce da sola con livelli d'ingresso alti e frequenze di taglio basse, così gli sweep completi restano sicuri per le casse.

**HiPass** Aggiunge la banda passa-alto all'uscita.

**BandPass** Aggiunge la banda passa-banda all'uscita.

**LoPass** Aggiunge la banda passa-basso all'uscita.

**Frequency** La frequenza di taglio, da 20 Hz a 20 kHz. Il grande sweep.

**FilterOn** Inserisce o esclude il filtro. Da disattivato è un filo dritto e pulito.

**Mono** Mette in serie i filtri dei due canali a partire dall'ingresso sinistro, per una pendenza di 24 dB e una risonanza doppia, e alimenta entrambe le uscite.

**MixInvert** Fa uscire il segnale secco meno quello filtrato, per effetti di cancellazione che stanno bene prima di un delay o di un riverbero.

## Ricetta

**Sweep sincronizzato** LoPass attivo, Resonance 6, Drive 3, Frequency 400. LfoSync attivo, LfoBeats 4, LfoWave Triangle, LfoDepth 6. Collega un organism Drums all'ingresso e la frequenza di taglio si apre e si chiude una volta per battuta, a tempo con il transport.

## Organism correlati

LFO, Gain, Follower
