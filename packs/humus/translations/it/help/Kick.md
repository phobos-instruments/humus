# Kick

Una cassa da club sintetizzata, con un inviluppo di pitch, un decadimento del corpo, un clic di rumore e uno stadio di saturazione.

Una sinusoide accordata su Tune parte più acuta di un intervallo fissato da Punch e scende nel tempo di PitchDecay, il suo corpo svanisce nel tempo di Decay, un breve burst di rumore aggiunge il transiente e uno stadio di saturazione la arrotonda: la cassa è progettata, non campionata. Si attiva in tre modi che si sommano: il pulsante Hit, qualsiasi note-on all'ingresso MIDI da un PianoRoll o da uno Steps, e 4/4, che la fa partire su ogni beat del transport mentre suona. Collega l'uscita a un Compressor o a una Console, e un Follower su di essa a un Gain per un ducking in sidechain.

## Parametri

**Tune** La frequenza di riposo del corpo, da 30 a 90 Hz. Il riquadro accanto alla manopola la mostra come nota: cliccalo e premi un tasto, su una tastiera MIDI o sul pianoforte che si apre, per accordare la cassa su quella nota.

**Punch** Di quanto il pitch parte sopra Tune. Più alto, attacco più duro.

**PitchDecay** P.Dec: quanto velocemente il pitch scende fino a Tune, in millisecondi. Corto è un tick, lungo è un laser.

**Decay** Decadimento del corpo in millisecondi, la lunghezza del boom.

**Click** Livello del transiente di rumore, per farsi strada in un mix denso.

**Wood** Un breve colpo di due parziali inarmoniche che seguono Tune, che svanisce in circa 35 ms. Con poco aggiunge un carattere da battente, con tanto si avvicina a un woodblock. A 0 resta il suono semplice.

**Drive** Saturazione. Spingila per un rombo distorto.

**Level** Livello d'uscita.

**FourFloor** 4/4: fa partire la cassa su ogni beat mentre il transport suona.

**Trigger** Hit: fa partire la cassa una volta. Mappalo su un pad.

## Ricetta

**Cassa dritta** Tune 48, Punch 4, PitchDecay 40, Decay 420, Click 0.35, Drive 0.3, 4/4 attivo. Premi Riproduci e la cassa segue il transport; collega un Follower alla sua uscita e l'env del Follower a un Gain sul basso per il ducking.

## Organism correlati

SideKick, Microdot, Steps
