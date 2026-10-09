# Phaser

Un phaser a sei stadi i cui notch si spostano tra due frequenze.

Una catena di filtri passa-tutto lascia stare il livello e sposta solo la fase; quando rimescoli il risultato con il segnale secco, nei punti in cui i due non vanno d'accordo si annullano formando dei notch, e uno sweep sinusoidale sposta quei notch tra gli estremi di FrequencyRange. Siccome niente viene ritardato nel tempo, i transienti restano al loro posto, quindi può stare su un intero submix senza impastarlo. Mentre i notch di un Flanger sono equidistanti e metallici, quelli di un phaser sono pochi e irregolari, ed è lo sweep più sottile e più vocale. Collegalo dopo un piano elettrico, un pad o un bus del Mixer.

## Parametri

**FrequencyRange** Gli estremi basso e alto dello sweep. Un intervallo stretto è un lento ondeggiare del timbro; uno ampio è lo sweep completo, dal basso all'alto.

**Rate** Quanto velocemente i notch viaggiano tra gli estremi, in Hz.

**Feedback** Rimanda l'uscita all'ingresso, affilando i notch in picchi risonanti.

**Depth** Quanto segnale sfasato viene mescolato con quello secco. Zero è tutto secco; i notch più profondi stanno verso il centro, dove i due sono bilanciati.

## Ricetta

**Sweep lento sul pad** FrequencyRange da 200 a 2000, Rate 0.15, Feedback 0.5, Depth 0.5. Collegaci un pad tenuto di Rhizome e lo sweep ci mette circa sette secondi per attraversare una volta; spingi Feedback verso 0.8 per un picco più affilato.

## Organism correlati

Flanger, Chorus, Filter
