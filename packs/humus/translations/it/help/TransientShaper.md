# TransientShaper

Modella l'attacco e il sostegno di un suono indipendentemente dal suo livello.

Sull'ingresso girano un inseguitore di inviluppo veloce e uno lento, e la loro differenza normalizzata dice se il segnale è in fase di attacco o di coda, a prescindere da quanto è forte, così una ghost note leggera viene modellata quanto un colpo pieno. Attack agisce sull'attacco e Sustain sulla coda, ciascuno fino a 15 dB in un senso o nell'altro. Il rilevamento è unico per i due canali stereo, così l'immagine non si sposta. Anche i rientri e il rumore di fondo vengono modellati, quindi su una sorgente rumorosa metti prima un Gate. Collegaci un organism Drums, un loop di Sampler o un Kick.

## Parametri

**Attack** Modella l'attacco. Positivo esagera il transiente per dare più mordente; negativo lo ammorbidisce. Se comincia a fare clic, riduci qui invece di toccare OutputGain.

**Sustain** Modella il corpo e la coda. Positivo li allunga e fa emergere l'ambiente; negativo li accorcia per un suono secco e serrato senza bisogno di un gate.

**OutputGain** Recupera il livello dopo la modellazione.

## Ricetta

**Batteria più serrata** Attack 0.3, Sustain -0.4, OutputGain 1 su un loop di batteria da un Sampler. I colpi prendono mordente e l'ambiente tra uno e l'altro sparisce. Qui piccoli movimenti fanno molto, perché l'effetto non resta mai senza headroom come fa un compressore.

## Organism correlati

SideChain, Console, Kick, Sampler
