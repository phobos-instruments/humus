# SpectralFilter

Un modellatore spettrale che inclina il bilanciamento timbrico e rende lo spettro più netto o più morbido rispetto al suo stesso inviluppo.

Dove un EQ applica una curva fissa, SpectralFilter misura l'ingresso banda per banda e scala ognuna rispetto all'inviluppo spettrale locale. Tilt è una pendenza di brillantezza con una sola manopola e Contrast è un compressore spettrale: sotto zero avvicina i picchi a ciò che li circonda, sopra zero li solleva. La fase resta intatta, così con Tilt 0 e Contrast 0 è trasparente, e il percorso secco viene ritardato della stessa quantità, così qualsiasi Mix resta coerente in fase. Il selettore Mode sceglie Stereo o Mono. Aggiunge latenza. Collegalo dopo un Substrate, un Sampler o un intero Mixer.

## Parametri

**Tilt** Una pendenza di guadagno lungo la banda, fino a circa 18 dB in ciascun senso. Sotto zero scurisce alzando le basse e tagliando le alte; sopra zero schiarisce.

**Contrast** Quanto ogni banda viene allontanata dall'inviluppo spettrale o avvicinata a esso. Negativo ammorbidisce e sfuma un suono affollato; positivo rende i picchi più netti rispetto alle valli, per un risultato più nitido e articolato.

**Mix** Quanto segnale elaborato si sente rispetto a quello secco.

## Ricetta

**Presenza senza boost** Tilt 0, Contrast 0.3, Mix 1 su un pad opaco da un Substrate. Vengono sollevate le bande che hanno già un picco invece di una frequenza fissa, così il pad guadagna definizione senza una nuova risonanza. Collega un LFO a Contrast per un lento respiro spettrale.

## Organism correlati

SpectralFreeze, SpectralMorph, Filter, Console
