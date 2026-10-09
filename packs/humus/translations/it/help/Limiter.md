# Limiter

Un limiter brickwall con attacco istantaneo: niente supera il tetto.

Threshold è dove inizia la limitazione e Ceiling è fin dove può arrivare l'uscita, e l'uscita viene scalata in modo che un suono che arriva alla soglia esca al tetto. Abbassare Threshold quindi rende il risultato più forte e più limitato insieme, ed è questa la manopola da muovere; Ceiling è la linea di sicurezza che imposti una volta sola. L'attacco è istantaneo e un taglio netto al tetto gli copre le spalle, così tutta l'espressività sta nel rilascio. Il rilevamento è collegato tra i canali, così la limitazione non sbilancia mai l'immagine stereo. Collegalo per ultimo, dopo una Console o un Mixer e prima del SoundOut.

## Parametri

**InputGain** Regola il suono in arrivo prima della limitazione.

**Threshold** Dove inizia la limitazione. Abbassala per avere più limitazione e un risultato più forte.

**Ceiling** Il livello più alto ammesso in uscita. Lascia un po' di headroom sotto il fondo scala per quello che viene dopo.

**HoldTime** Per quanto la riduzione massima viene tenuta prima che inizi il rilascio, in millisecondi. Con un valore piccolo eviti il chattering sui picchi ravvicinati.

**ReleaseTime** Quanto in fretta il limiter si riprende dopo un picco, in millisecondi. Corto tiene alto il volume ma può pompare sul materiale denso; lungo resta pulito e cede un po' di livello.

## Ricetta

**Tetto sul master** Collega l'uscita della Console al Limiter e il Limiter al SoundOut. Ceiling 0.95, ReleaseTime 150, HoldTime 10, poi abbassa Threshold finché la sezione più forte non mostra qualche dB di riduzione e non di più.

## Organism correlati

Compressor, Console, VuMeter
