# Compressor

Un compressore feed-forward con un ginocchio morbido, uno stadio di hold e un rilascio sincronizzato al tempo.

Misura l'ingresso, decide un guadagno e lo applica, così i momenti forti vengono abbassati e la distanza tra il più forte e il più debole si accorcia. Il rilevamento è collegato tra i canali: si calcola un solo guadagno e lo si applica a ogni lato, così un momento forte a sinistra non può spostare l'immagine verso destra. Imposta prima Threshold, poi CompressionRatio, poi AttackTime e ReleaseTime, e MakeupGain per ultimo. Su una parte con una pulsazione, attiva Sync: così il recupero cade su una suddivisione del beat e ci resta quando il tempo cambia. Non c'è un ingresso di sidechain; per comprimere un suono in base a un altro, usa SideChain.

## Parametri

**InputGain** Regola il suono in arrivo prima di tutto il resto. Alzandolo spingi più suono oltre la soglia, senza spostare la soglia.

**MakeupGain** Guadagno aggiunto dopo la riduzione, per riportare su il suono livellato.

**AutoMakeupGain** Calcola il guadagno di compensazione da Threshold e CompressionRatio, così cambiare uno dei due non cambia anche il livello. Comodo mentre cerchi la regolazione giusta.

**AttackTime** Quanto velocemente risponde una volta superata la soglia, in millisecondi. Veloce cattura il colpo; lento lascia passare il colpo e livella quello che segue.

**HoldTime** Per quanto tempo la riduzione piena viene mantenuta prima che inizi il rilascio, in millisecondi. Impedisce al compressore di mollare e riafferrare il suono su un materiale che cala per un attimo.

**ReleaseTime** Quanto velocemente smette quando il suono torna sotto la soglia, in millisecondi. Viene ignorato mentre Sync è attivo.

**CompressionRatio** Con quanta forza viene tenuto il suono. 2 significa che un suono 2 dB sopra la soglia finisce 1 dB sopra. Rapporti bassi livellano, rapporti alti schiacciano.

**Threshold** Il livello in cui inizia la compressione. Sopra, il suono viene tenuto giù; sotto, non succede niente.

**KneeWidth** Quanto gradualmente arriva il rapporto, in dB da entrambi i lati della soglia. Un ginocchio ampio entra piano invece di scattare.

**Sync** Lega il rilascio al transport invece che a ReleaseTime. L'attacco resta dove l'hai messo.

**SyncUnit** Il valore di nota in cui conta il rilascio sincronizzato, per esempio 1/8.

**SyncMultiplier** Quante di quelle unità fanno un rilascio.

## Ricetta

**Colla sul bus della batteria** Threshold 0.4, CompressionRatio 3, KneeWidth 8, AttackTime 20, HoldTime 0, Sync attivo con SyncUnit 1/8. Collegaci la somma della batteria di un Mixer, attiva AutoMakeupGain mentre imposti la soglia, poi disattivalo quando il livello è dove lo vuoi.

## Organism correlati

SideChain, Limiter, NoiseGate, TransientShaper
