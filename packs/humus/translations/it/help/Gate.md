# Gate

Un gate audio che si apre con il livello di un segnale, con note MIDI tenute o con un interruttore.

Il segnale al primo ingresso passa quando il gate è aperto e scende a Floor quando è chiuso. Source decide cosa lo apre: LEVEL ascolta l'ingresso key, o il segnale stesso quando lì non è collegato niente, e apre sopra Threshold; MIDI lo tiene aperto finché una nota qualsiasi è tenuta all'ingresso MIDI; SWITCH affida il gate all'interruttore Open. Lo stato di apertura, smussato, viene pubblicato come valore di controllo "open", così lo stesso gate può far scattare o pilotare qualsiasi altra cosa nella patch. Collega un Kick all'ingresso key e un pad all'ingresso del segnale per tagliuzzarlo in sidechain, o un PianoRoll all'ingresso MIDI per suonare il gate come uno strumento.

## Parametri

**Source** Cosa apre il gate: LEVEL, MIDI o SWITCH.

**Detector** Come ascolta LEVEL. PEAK reagisce alle escursioni istantanee, RMS al volume medio.

**Threshold** Il livello che apre il gate in modalità LEVEL.

**AttackTime** Quanto velocemente si apre il gate, in millisecondi.

**HoldTime** Quanto a lungo il gate resta aperto dopo che la sorgente cala, in millisecondi.

**ReleaseTime** Quanto lentamente si chiude il gate, in millisecondi.

**Floor** Quanto segnale filtra ancora quando è chiuso. Zero è un gate netto; alzalo per un effetto da espansore.

**Duck** Scambia aperto e chiuso, e così fa ducking con una qualsiasi delle tre sorgenti.

**Open** L'interruttore manuale, usato quando Source è SWITCH. Collegaci uno Slider, un LFO o un Follower per far scattare il gate da un controllo.

## Ricetta

**Pad tagliuzzato** Collega un pad al primo ingresso e un Kick all'ingresso key. Source LEVEL, Detector PEAK, Threshold 0.2, AttackTime 1, HoldTime 60, ReleaseTime 80, Floor 0. Ora il pad suona solo su ogni colpo di cassa; attiva Duck e invece sparirà su ogni colpo di cassa.

## Organism correlati

NoiseGate, SideChain, Gain, Follower
