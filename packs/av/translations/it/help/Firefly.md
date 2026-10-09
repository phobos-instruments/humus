# Firefly

Un lampo sull'immagine, a tempo, a mano o dalle note.

Un ingresso video e un'uscita: quello che entra passa, e il lampo ci si sovrappone. Senza niente collegato il lampo cade sul nero, così Firefly da solo è una lampada. Scatta sulla griglia del transport, a una velocità libera tutta sua, dal pulsante Flash o dalle note che arrivano al suo ingresso MIDI, così un MIDI Track, un DNA o un Riff sulla timeline lo pilotano come qualsiasi altra cosa. La velocity della nota stabilisce quanto è forte quel lampo. Hue e Saturation gli danno un colore, e siccome sono parametri come gli altri, un LFO su Hue fa scorrere il colore mentre suoni. Ogni lampo è temporizzato dal clock dell'immagine stessa, così anche un lampo più corto di un fotogramma si vede. Un avvertimento da prendere sul serio: i lampi veloci possono scatenare crisi nelle persone con epilessia fotosensibile. Tienili lenti davanti a un pubblico che non conosci, e avvisa che stanno per arrivare.

## Parametri

**Mode** Che cosa lo fa scattare da solo. Beat scatta sulla griglia del transport e segue il brano, a riposo mentre il transport è fermo. Free scatta alla velocità di Rate, che il brano suoni o no. Played non scatta mai da solo: lo fanno solo il pulsante Flash e le note all'ingresso MIDI, così le luci seguono le tue mani o un MIDI Track e nient'altro. Il pulsante e le note lo fanno scattare in ogni modalità.

**Division** Quanto spesso scatta la modalità Beat rispetto al beat: da una nota intera a un trentaduesimo, più terzine e valori puntati.

**Rate** Lampi al secondo in modalità Free, da un impulso lento a 0.05 Hz fino a 30 Hz.

**Width** Quanto dura ogni lampo, come quota dello spazio tra un lampo e l'altro; in modalità Played lo spazio è un beat al tempo del brano. Corto è il flash di una macchina fotografica; largo resta acceso tanto quanto resta spento.

**Hue** Il colore del lampo in gradi sulla ruota dei colori. Si vede solo quando Saturation è alzata.

**Saturation** Quanto è colorato il lampo. 0% è bianco, 100% è la tinta piena.

**Level** Quanto il lampo copre l'immagine. 100% la sostituisce per tutta la durata del lampo.

**Flash** Fa scattare un lampo a mano. Mappalo su un pad o su un pedale e suona le luci.

## Ricetta

**Lampo a tempo su un video** Collega un VideoPlayer a Firefly e Firefly a un VideoOut. Mode Beat, Division 1/4, Width 12%, Shape 40%, Saturation 0%. Premi Riproduci e l'immagine lampeggia a ogni beat.

**Suonato dalla timeline** Imposta Mode su Played, collega un MIDI Track all'ingresso MIDI di Firefly e scrivi una linea di note. Le note forti lampeggiano forte, quelle piano si vedono appena, così le luci leggono la parte che hai scritto.

## Organism correlati

VideoFX, Lumen, VideoOut
