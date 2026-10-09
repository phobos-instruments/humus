# Bloom

Il riverbero shimmer: un tank di riverbero modulato con un pitch shifter dentro il suo loop di feedback.

Ogni giro nel tank viene trasposto di nuovo, così una coda sale di un'ottava, poi di due, finché lo smorzamento non se la porta via. Il bilanciamento tra il feedback semplice e quello trasposto è Shimmer: a zero è un riverbero normale, al massimo la coda è tutta salita e niente corpo. Feedback stabilisce quanto a lungo risuona il tutto. Porta Shift in negativo e invece la coda scende. Collegalo dopo un Rhizome, un Wave o un Sampler, e mettici prima un Fern per una coda che fa anche eco.

## Parametri

**Mix** Il segnale secco contro quello effettato.

**Shift** Di quanto il pitch shifter traspone ogni giro, in semitoni. 12 è la classica ottava, 7 impila quinte, -12 scende.

**Shimmer** Quanto segnale trasposto rientra nel tank rispetto alla coda non trasposta.

**Feedback** Quanto del tank torna al suo stesso ingresso. È la lunghezza della coda.

**Size** Le dimensioni della stanza, da piccola a immensa.

**Diffusion** Quanto in fretta i primi echi si confondono tra loro. Bassa lascia sentire echi distinti dentro la coda.

**LowCut** Toglie le basse frequenze dall'ingresso del tank, così la coda che cresce non invade il basso.

**HighCut** Scurisce la coda a ogni giro. Più basso dà una coda più corta e più calda.

**ModRate** La velocità del movimento dentro il tank, in Hz.

**ModDepth** Quanto si spinge quel movimento. Poco tiene viva la coda, tanto la trasforma in acqua.

**Mode** Il tipo di stanza. Bloom è piccola e vicina, Hall è naturale, Cathedral è immensa, Cloud sta nel mezzo con più diffusione.

**Voice** La disposizione del pitch shifter. Single segue Shift, Dual aggiunge una seconda voce che va nella direzione opposta, Stacked aggiunge una seconda voce con il doppio della trasposizione, e Off è un riverbero semplice qualunque cosa dica Shift.

**Color** L'inclinazione timbrica della coda. Bright tiene aperte le copie trasposte, Dark le smorza, Neutral sta nel mezzo.

## Ricetta

**Alone sul pad** Mode Cathedral, Voice Single, Shift 12, Shimmer 0.6, Feedback 0.6, HighCut 6000, Color Dark, Mix 0.4. Collegaci un pad lento di Rhizome e tieni un accordo per una battuta o due mentre le ottave si accumulano.

## Organism correlati

Fern, Prism, SpectralFreeze
