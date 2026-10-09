# Crossover

Un divisore di frequenza che manda un segnale in uscita diviso in bande, una coppia stereo di uscite per banda, a partire dalla più bassa.

I punti di divisione sono allineati in fase, quindi se colleghi tutte le bande a un unico ingresso si risommano nel suono originale, con risposta piatta. Tutto quello che succede tra la divisione e la riunione riguarda una sola banda: un Compressor su ogni banda è una dinamica multibanda, un Fern solo sui medi è un'eco da cui il basso resta fuori, un Follower sull'uscita bassa trasforma la cassa in un segnale di controllo che gli hi-hat non toccano. Un nuovo Crossover è a tre vie; il menu a tendina Bands nel suo editor lo rimodella sul posto tra 2 e 5, mantenendo i cavi e le impostazioni dei punti di divisione, e man mano che aggiungi bande compaiono Freq3 e Freq4. Elabora le bande e riuniscile, ma evita di mescolare il risultato riunito con il segnale secco da cui è nato: un crossover ruota la fase, quindi i due si cancellano in modo strano se li sovrapponi.

## Parametri

**Freq1** La frequenza di crossover più bassa, in Hz.

**Freq2** La frequenza di crossover successiva verso l'alto. I punti di divisione mantengono il loro ordine, quindi non puoi trascinarne uno sotto quello precedente.

**Slope** Quanto è ripida la giunzione tra le bande. 24 dB/oct è lo standard, allineato in fase e con somma piatta. 12 dB/oct è più dolce, per fondere i timbri; in questa modalità le bande alterne hanno la polarità invertita così la somma resta piatta, cosa che da sola non si sente. 48 dB/oct è quasi un muro, così puoi spingere forte ogni banda per conto suo.

## Ricetta

**Batteria in tre bande** Freq1 120, Freq2 2000, Slope 24 dB/oct. Collega in ingresso un Mixer di batteria, un Compressor su ogni coppia di uscite con un attacco più lento sulla banda bassa, e tutti e tre i Compressor all'ingresso successivo, che li risomma.

## Organism correlati

ParaEQ, Filter, StereoTool, Follower
