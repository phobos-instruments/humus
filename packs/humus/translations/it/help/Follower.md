# Follower

Un inseguitore di inviluppo che trasforma il volume di un segnale in un valore di controllo e in un gate.

Raddrizza l'audio al suo ingresso e lo leviga in un profilo, pubblicato come valore di controllo "env" sulla prima uscita, circa da 0 a 1 per un audio a fondo scala e scalato da Gain. Una seconda uscita e il valore di controllo "gate" restano a 0 finché il profilo non supera Threshold, poi restano a 1 finché il livello non è sceso sotto metà della soglia e Hold non è scaduto. Ogni apertura manda anche Note dall'uscita MIDI, con una velocity presa dal livello del colpo. Collega env a un Gain per un ducking o un expander, alla Frequency di un Filter per un auto-wah, o l'uscita MIDI a un VideoPad per tagliare le immagini sul beat. Per una dinamica completa in un solo organism, vedi SideChain e la famiglia dei Compressor.

## Parametri

**Attack** Quanto velocemente il profilo sale su un colpo, in millisecondi. Corto cattura i transienti, lungo li leviga in un livello.

**Release** Quanto lentamente il profilo ricade dopo, e questo decide per quanto un suono tiene aperto il gate dopo che si è fermato.

**Gain** Scala il profilo così che la sua escursione corrisponda a quello che pilota.

**Threshold** Il livello che il profilo deve raggiungere per aprire il gate. 0 disattiva il gate.

**Hold** Il tempo minimo in cui il gate resta aperto dopo che un colpo l'ha aperto, in millisecondi, così un solo colpo di batteria non si spezzetta in più note.

**Note** La nota MIDI inviata a ogni apertura del gate.

## Ricetta

**Tagli guidati dalla cassa** Collega una cassa al Follower, Attack 2, Release 150, Threshold 0.3, Hold 100. Collega la sua uscita MIDI a un VideoPad e ogni cassa fa partire il pad; collega intanto env al Gain di un synth pad e il synth si abbassa sotto ogni colpo.

## Organism correlati

Gain, SideChain, Filter, Button
