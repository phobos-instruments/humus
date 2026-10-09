# SpectralFreeze

Un freeze e smear spettrale a phase vocoder, che tiene fermo uno spettro lasciandolo in movimento.

Smear allunga quanto persiste il livello di ogni banda da un frame di analisi all'altro, e Freeze blocca lo spettro corrente e ignora l'ingresso finché non lo rilasci. Le bande tenute conservano l'avanzamento di fase misurato sulla sorgente, così un accordo congelato continua a battere e a muoversi. Diffusion sfuma lo spettro tenuto sulle bande vicine e aggiunge una lenta deriva di fase; Shimmer ci rimescola dentro una copia un'ottava sopra. Con Smear a 0 e Freeze disattivato il segnale passa identico, e il percorso secco viene ritardato per restare allineato. Collegaci un Substrate, un Sampler o una voce e manda l'uscita a un Verbatim o a un Fern.

## Parametri

**Freeze** Afferra lo spettro che sta suonando in questo momento e lo tiene, ignorando l'ingresso finché non lo disattivi.

**Smear** Quanto persiste il livello di ogni banda, da circa un millisecondo a dieci secondi. A 0 l'ingresso passa invariato; più in alto cresce una coda spettrale sempre più lunga. È la manopola da automatizzare.

**Diffusion** Sfuma i livelli tenuti tra le bande e aggiunge una lenta camminata casuale della fase, così un freeze sboccia invece di restare immobile. Aggiungilo dopo aver congelato, altrimenti spalma la sorgente prima che tu riesca a catturarla.

**Shimmer** Rimescola dentro una copia un'ottava sopra dello spettro tenuto.

**Mix** Quanto si sente il segnale congelato rispetto al segnale secco.

## Ricetta

**Drone sospeso** Smear 0.7, Diffusion 0.3, Shimmer 0.2, Mix 1. Collegaci un Sampler o un pad tenuto, suona un accordo e attiva Freeze nel momento che vuoi tenere; mappa Freeze su un Button o su una nota MIDI per afferrarlo a tempo. Disattiva Freeze per far ripassare l'ingresso.

## Organism correlati

SpectralFilter, SpectralMorph, SoundSpace, Substrate
