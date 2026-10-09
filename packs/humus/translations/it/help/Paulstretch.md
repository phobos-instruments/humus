# Paulstretch

Un time stretch estremo che trasforma un file audio in una nuvola lenta e senza fase.

Segue l'algoritmo [Paulstretch](https://github.com/paulnasca/paulstretch_python) di Nasca Octavian Paul, di pubblico dominio. Non è uno stretcher per lavorare sul tempo: sotto le quattro volte circa impasta il suono, ed è proprio l'impasto lo strumento. La testina striscia nel file a una velocità pari a uno diviso Stretch; a ogni passo si applica una finestra, lo spettro viene conservato e le fasi vengono estratte di nuovo, così l'armonia resta e l'istante se ne va. I due canali estraggono le loro fasi in modo indipendente, così un file mono esce largo. Dagli un accordo, una voce o un brano intero e collega l'uscita a un Fern o a un Verbatim per un tappeto, o direttamente a un Mixer. La barra del transport sotto il file ti fa spostare nella sorgente, e ogni suo pulsante è un parametro, quindi fai clic destro su uno qualsiasi per MIDI learn, OSC learn, Controlla con o Segui. Lo Stop del transport lo lascia suonare; premi Stop una seconda volta, quando non sta andando niente, e si ferma insieme a tutto il resto.

## Parametri

**File** Il file audio sorgente.

**Stretch** Quante volte più lento del tempo reale, da 1 a 100. Otto è l'impostazione classica; a cinquanta si muove appena.

**Window** La finestra di analisi in secondi. Corta conserva un po' di ritmo e di gorgoglio, lunga dissolve tutto in un pad.

**Loop** Fa ripartire la sorgente da capo, così la nuvola non finisce mai. Disattivato la suona una volta e poi si spegne nel silenzio.

**Active** Attiva la riproduzione. Disattivato è muto.

**Level** Guadagno d'uscita.

## Ricetta

**Un pad da un accordo** Carica un accordo di due secondi, Stretch 30, Window 0.5, Loop attivo. Collega l'uscita a un Verbatim con una coda lunga e abbassa Level finché non sta sotto il resto della patch.

## Organism correlati

SpectralFreeze, FilePlayer, Bloom
