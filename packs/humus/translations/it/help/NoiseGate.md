# NoiseGate

Un gate che lascia passare il suono finché è abbastanza forte e si chiude quando non lo è, oppure fa il contrario e lo abbassa in ducking.

Toglie il soffio, il ronzio e i rientri che vivono negli spazi tra una frase e l'altra, rende più secco un tamburo accorciandone la coda e in modalità Duck tiene giù il suono mentre supera la soglia. Il controllo Threshold ha due maniglie perché il gate ha due livelli: quello alto lo apre, quello basso lo chiude, e tenerli distanti evita il chattering del gate su un segnale che resta in bilico sulla linea. Il rilevamento è comune a tutti i canali, così non taglia mai via un lato dall'immagine stereo. Non c'è un ingresso key: per aprire e chiudere un suono col ritmo di un altro, usa SideChain. Collegalo dopo un SoundIn o prima di un Compressor.

## Parametri

**Mode** Off lascia passare i suoni forti e attenua quelli deboli. Duck fa il contrario, tenendo giù il suono mentre supera la soglia.

**InputGain** Regola il suono in ingresso prima del rilevamento, così puoi spingere il materiale oltre la soglia senza spostarla.

**Threshold** Il livello di apertura sulla maniglia alta e quello di chiusura sulla bassa. Tienili distanti per evitare il chattering.

**Range** Di quanto viene attenuato il suono mentre il gate è chiuso. Tutto in basso non passa niente; un po' più su lascia gli spazi attenuati invece che vuoti.

**AttackTime** Quanto velocemente si apre il gate, in millisecondi. Abbastanza veloce da non smussare una nota, abbastanza lento da non fare clic.

**HoldTime** Quanto resta aperto il gate dopo che il segnale scende sotto il livello di chiusura, in millisecondi. Lascia a un tamburo il suo corpo prima che il gate si chiuda.

**ReleaseTime** Quanto velocemente si chiude il gate una volta finito l'hold, in millisecondi. Corto è brusco e ritmico, lungo sfuma la coda.

## Ricetta

**Rullante serrato** Collega il rullante all'ingresso. Threshold apertura 0.3 e chiusura 0.15, Range 0, AttackTime 0.5, HoldTime 40, ReleaseTime 60. Abbassa HoldTime finché la coda non si accorcia, e alza un po' Range se gli spazi suonano vuoti.

## Organism correlati

SideChain, Compressor, SideKick, TransientShaper
