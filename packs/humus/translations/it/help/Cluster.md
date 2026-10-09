# Cluster

Otto slot di accordi che partono come voicing completi da un'uscita MIDI.

Ogni slot contiene una manciata di note scritte per nome, come C3 E3 G3, e accetta anche diesis, bemolle e semplici numeri di nota. Uno slot parte quando premi il suo pulsante, suoni la sua nota trigger o guidi il suo parametro Fire da un collegamento di controllo. In modalità PADS gli otto slot stanno su otto note in arrivo a partire da TriggerNote e suonano alla velocity con cui suoni. In modalità FOLLOW uno slot diventa una forma che segue quello che suoni: ogni nota in arrivo diventa la fondamentale e l'accordo si sposta con lei. Le note condivise tra due slot che suonano vengono contate, quindi rilasciare un pad non taglia mai una nota che un altro tiene ancora. Collega l'uscita a un Rhizome, a un Wave o a qualsiasi altro strumento.

## Parametri

**Mode** PADS suona gli otto slot da otto tasti. FOLLOW traspone uno slot a partire da quello che suoni.

**Hold** Cosa significa una pressione. GATE fa suonare l'accordo finché il trigger è premuto. LATCH rende ogni trigger un interruttore, così i pad si possono sovrapporre. PEDAL suona un accordo alla volta: ogni pressione sostituisce la precedente e i rilasci vengono ignorati, quindi uno slot vuoto diventa un pulsante di stop.

**TriggerNote** La nota in arrivo associata allo slot 1. Le sette note successive verso l'alto vanno agli slot da 2 a 8.

**Slot** Quale slot si porta dietro la modalità FOLLOW.

**Velocity** La velocity usata quando uno slot parte dal suo pulsante o da un collegamento di controllo.

**Notes1** Le note dello slot 1, scritte come nomi o numeri. E così via da 2 a 8.

**Fire1** Il trigger dietro il pulsante dello slot 1, raggiungibile da collegamenti di controllo e automazione. E così via da 2 a 8.

## Ricetta

**Cambi con un dito** Mode PADS, Hold PEDAL, TriggerNote 36. Scrivi una progressione negli slot da 1 a 4 e lascia vuoto lo slot 5. Collega un PianoRoll all'ingresso e Cluster a un Rhizome, poi disegna singole note da 36 a 39 dove deve cambiare ogni accordo e un 40 dove deve fermarsi.

## Organism correlati

PianoRoll, Steps, Riff, Harmonizer
