# Microdot

Un synth di basso rolling in levare, pilotato da una griglia di trigger a sedici step agganciata al transport.

Ogni colpo è un oscillatore a banda limitata, con un'onda quadra opzionale un'ottava sotto, che passa per un inviluppo di filtro veloce e uno stadio di saturazione. Un organism appena creato parte con il roll classico, tre sedicesimi dopo ogni battere, così si aggancia appena il transport suona, ed è muto quando è fermo. La griglia fa sempre da gate; Seq decide chi comanda il pitch. Con Seq attivo ogni colpo suona il campo Note. Con Seq disattivato i colpi seguono la nota che l'ingresso MIDI sta tenendo, così la linea di un Riff, di uno Steps o di un PianoRoll collegato viene ritagliata dalla griglia. Abbinalo a un Kick in 4/4 e collega entrambi a un Compressor.

## La griglia

Clicca uno step per attivarlo o disattivarlo; la colonna su cui si trova il transport è illuminata. La riga ^ alza di un'ottava il colpo di quello step, e la riga di tenuta in basso prolunga la nota precedente attraverso quello step, così più step di fila tengono una sola nota. Clic destro per i pattern pronti: Roll -111, Offbeat --1-, Full 1111, Random e Clear. Il pattern viene salvato nella patch.

## Parametri

**Note** Il pitch che suona ogni colpo mentre Seq è attivo.

**Wave** SAW, SQR, PLS (un impulso stretto), TRI, SIN o FAT, una coppia di denti di sega scordati tra loro per dare larghezza.

**Seq** Attivo, il pitch lo decide il campo Note; disattivato, lo decide la nota MIDI tenuta. La griglia fa da gate in entrambi i casi.

**Cutoff** La frequenza di riposo del filtro.

**Reso** Risonanza. Alzala e lo sweep dell'inviluppo comincia a cantare.

**EnvMod** Di quanto l'inviluppo apre il filtro sopra Cutoff a ogni colpo.

**Decay** Lunghezza dell'inviluppo del filtro in millisecondi; modella anche il livello della nota. Da 80 a 150 mantiene il carattere rolling e serrato.

**Gate** Durata del colpo come frazione dello step. Più corto è più saltellante.

**Sub** Onda quadra un'ottava sotto, per dare peso.

**Drive** Saturazione dopo il filtro.

**Level** Livello d'uscita.

**SwingFollow** Follow: prende il groove della patch dal transport invece che dallo slider Swing.

**Swing** Ritarda un sedicesimo su due, fino a un andamento terzinato. Usato solo mentre Follow è disattivato.

## Ricetta

**Basso rolling** Note A1, Wave SAW, Cutoff 700, Reso 0.35, EnvMod 0.5, Decay 120, Gate 0.8, Sub 0.4, Drive 0.35, Follow attivo. Aggiungi un Kick con 4/4 attivo e premi Riproduci; la cassa cade sul beat e il basso scorre tra un colpo e l'altro. Disattiva Seq e collega un Riff per dare al roll una linea che si muove.

## Organism correlati

Kick, Acid, Riff, DNA
