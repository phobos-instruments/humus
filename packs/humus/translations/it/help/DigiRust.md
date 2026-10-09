# DigiRust

Un canale lo-fi che imita un convertitore economico che si sta guastando: meno bit, un clock più lento, un clock che vacilla e fruscio.

Bits riduce il segnale a meno livelli e Rate tiene ogni campione più a lungo, così gli acuti si ripiegano in aliasing e diventano grana sporca. Jitter fa vacillare il clock, così la durata di ogni campione cambia dall'uno all'altro, e Noise aggiunge un fruscio che segue il segnale e sparisce nel silenzio, così una patch a riposo resta silenziosa. Tone inclina il risultato verso un suono più scuro o più presente, Mix lo miscela con l'ingresso secco per una sporcatura in parallelo, e un filtro anti-DC tiene l'uscita centrata. Randomizza tira a sorte Bits, Rate, Jitter, Noise e Tone ma lascia stare Mix e Level, così un tiro cambia il colore, mai il bilanciamento della patch. Mettilo dopo un Sampler o un organism Drums, o prima di un Fern per un'eco degradata.

## Parametri

**Bits** Lunghezza di parola, da 1 a 16, a passi interi. Più basso è più grezzo.

**Rate** Clock di campionamento in Hz. In cima alla sua corsa è fuori dal percorso; basso è sgranato.

**Jitter** Vacillamento del clock. Zero è un clock stabile; alzandolo arrivano ondeggiamenti e sbavature.

**Noise** Fruscio che segue il segnale. Zero è silenzioso.

**Tone** Inclinazione da scuro a presente, attorno al centro.

**Mix** Miscela da secco ad arrugginito.

**Level** Livello d'uscita.

## Ricetta

**Batteria da vecchio campionatore** Bits 10, Rate 22000, Jitter 0.1, Noise 0.15, Tone 0.4, Mix 1. Collega un Sampler che suona un break e gli acuti si smorzano nel classico crunch dei primi campionatori; abbassa Rate a 8000 per il breakdown.

## Organism correlati

Sampler, Fern, Filter
