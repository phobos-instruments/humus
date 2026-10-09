# FrequencyShifter

Aggiunge un numero fisso di hertz a ogni frequenza del segnale.

Un pitch shifter moltiplica ogni frequenza per lo stesso rapporto, così le armoniche restano allineate e il suono mantiene la sua identità. Questo organism invece aggiunge a ognuna lo stesso numero di hertz: una serie a 100, 200 e 300 spostata di 50 diventa 150, 250 e 350, che non è più armonica. Piccoli spostamenti di pochi hertz danno uno shimmer lento, come un phasing, contro il segnale secco; spostamenti più ampi trasformano una nota in una campana o in un raschio. Funziona come shifter a banda laterale singola, così il risultato resta pulito invece di suonare come una modulazione ad anello, e lo stesso oscillatore pilota entrambi i canali, così l'immagine stereo regge. Collegalo dopo un Rhizome o un Sampler, o dopo un Fern così le ripetizioni scivolano via dalla nota.

## Parametri

**ShiftFrequency** Quanti hertz aggiungere, in positivo o in negativo. Gli spostamenti negativi avvicinano le parziali tra loro, e le armoniche basse possono essere spinte giù oltre lo zero e risalire.

**WetDryMix** Quanto del segnale spostato si sente rispetto all'originale. Piccoli spostamenti contro il segnale secco battono lentamente; tutto effettato, il pitch originale si perde del tutto.

## Ricetta

**Shimmer lento** Collega un pad sostenuto. ShiftFrequency 3, WetDryMix 0.5, e la copia spostata batte contro il pad secco circa tre volte al secondo. Collega un LFO a ShiftFrequency con un valore basso di Amplitude per far vagare il battimento.

## Organism correlati

Prism, SpectralMorph, Chorus
