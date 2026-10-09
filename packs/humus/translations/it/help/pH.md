# pH

Un sintetizzatore FM con quattro motori: un motore a sei operatori che legge i banchi .syx del DX7, ed emulazioni dei chip OPN2, OPM e OPL3.

È il banco a decidere il motore. Un .syx contiene voci a sei operatori; un .wopn, .tfi o .dmp contiene strumenti OPN2 per il core [Nuked-OPN2](https://github.com/nukeykt/Nuked-OPN2); un .opm contiene voci OPM per [Nuked-OPM](https://github.com/nukeykt/Nuked-OPM); un .wopl contiene strumenti OPL3 per [Nuked-OPL3](https://github.com/nukeykt/Nuked-OPL3). Caricane uno e lo suona il motore giusto: otto voci sul motore a sei operatori, sei canali sull'OPN2, otto sull'OPM e diciotto sull'OPL3. Il pitch arriva dal Tuning della patch, così una scala microtonale suona anche sulle voci dei chip. Una patch nomina il suo banco invece di contenerlo, quindi condividi il banco insieme alla patch. Collegaci un PianoRoll, un DNA o un MidiIn.

## Parametri

**Patch** Quale voce del banco suona, per nome. Quando scegli un banco si riparte dalla sua prima voce.

**Bright** Quanto forte i modulatori pilotano le loro portanti. Al centro è la voce così com'è scritta; in su aggiunge armoniche, in giù chiude il timbro verso una sinusoide.

**Attack** Quanto tempo impiega ogni operatore ad aprirsi, intorno a quello che chiede la voce.

**Release** Quanto tempo impiega la voce a sfumare dopo che sollevi il tasto.

**Detune** Scostamento tra gli operatori, in cent. Poco ispessisce una voce; tanto la fa battere contro se stessa.

**Vibrato** Profondità dell'oscillazione di pitch a bassa frequenza propria della voce.

**Speed** Quanto è veloce quell'oscillazione.

**Transpose** Sposta la nota in semitoni. Muove la frequenza che suona e non il numero di nota, così regge in qualsiasi accordatura.

**Fine** Sposta la nota in cent, allo stesso modo.

**Level** Livello d'uscita.

**BendRange** Fin dove arriva la rotella del pitch a fine corsa, in semitoni. Due è il valore di default più comune; zero ignora la rotella.

**Algorithm** Quale operatore pilota quale, e quali arrivano all'uscita. Il motore a sei operatori ne offre 32; i chip 8.

**Feedback** Quanto forte il primo operatore si ripiega su se stesso, da 0 a 7.

**Op1_Level** Livello d'uscita dell'operatore 1, e così via per gli operatori da 2 a 6. Quando scegli una voce in Patch viene caricata in queste righe, così quello che vedi è quello che suona.

**Op1_Ratio** Rapporto di frequenza dell'operatore 1 rispetto alla nota, e così via da 2 a 6.

**Op1_Detune** Scostamento fine dell'operatore 1, centrato su 7, e così via da 2 a 6.

**Op1_Attack** Velocità di attacco dell'inviluppo dell'operatore 1, e così via da 2 a 6.

**Op1_Decay** Velocità di decadimento dell'inviluppo dell'operatore 1, e così via da 2 a 6.

**Op1_Sustain** Livello di sostegno dell'inviluppo dell'operatore 1, e così via da 2 a 6.

**Op1_Release** Velocità di rilascio dell'inviluppo dell'operatore 1, e così via da 2 a 6.

**Op1_On** Toglie l'operatore 1 dalla voce o lo rimette, conservandone intanto il livello, e così via da 2 a 6. I chip hanno quattro operatori, quindi Op5_On e Op6_On non sono disponibili mentre è caricato un banco per chip.

**File** Il banco da cui suonare, e con esso il motore. Vuoto è il banco di fabbrica; un banco messo nella cartella dei banchi entra nell'elenco e prende il posto di un banco incluso con lo stesso nome.

## Ricetta

**Piano elettrico da chip** Carica il banco di fabbrica per chip e scegli in Patch un piano elettrico. Bright 0.6, Detune 6, Release 0.6, Vibrato 0.1, Speed 0.4. Collegaci un PianoRoll e dopo un Fern con Feedback intorno a 0.4 per l'eco; disattiva e riattiva Op4_On mentre suona per sentire cosa aggiunge l'operatore più in alto.

## Organism correlati

Rhizome, Wave, PianoRoll, DNA
