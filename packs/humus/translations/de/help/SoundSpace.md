# SoundSpace

Ein Granulator, den du spielst, indem du einen Cursor über eine Karte aus nach Klang sortierten Grains fliegen lässt, und der auch aufnimmt, was in ihn hineinkommt.

Lade bis zu vier Audiodateien; SoundSpace zerlegt sie in Grains, analysiert bei jedem Energie, Helligkeit, Rauschanteil und Bewegung und verteilt sie auf einer zweidimensionalen Karte, auf der ähnliche Grains nah beieinander landen, eine Farbe pro Datei. Der Cursor spielt eine Wolke aus den Grains um ihn herum, sodass ein Ziehen über die Karte ein Morphing zwischen Texturen ergibt. Misch sehr unterschiedliches Material, Stimme, Maschinen und Field Recordings, und die Karte zieht sich weiter auseinander, was die Reise über sie dramatischer macht. Rechtsklicke auf die Karte, um X und Y zu automatisieren oder per MIDI-Learn zuzuweisen, damit ein XY-Pad oder ein langsamer LFO sie abfliegen kann. Live ist zu Beginn eingeschaltet, also ist der Eingang schon eine eigene Quelle, und ein Kabel hinein ist alles, was es braucht: Die letzten acht Sekunden davon werden beim Eintreffen zerlegt und auf dieselbe Karte gelegt, als blasse Punkte zwischen den Farben der Dateien, die neuesten größer, sodass du siehst, wo dein Spiel landet, und den Cursor zwischen ihm und den Aufnahmen hin und her fliegen lassen kannst. Die Pegelanzeige neben Freeze zeigt den Pegel des Eingangs; eine leere Karte mit dunkler Anzeige heißt also, dass nichts ankommt. Beides lebt nebeneinander: Spiel etwas ein, während Dateien geladen sind, und die Karte hält beides, und Rec nimmt den Eingang in den nächsten leeren Datei-Slot auf, sodass eine Passage, die dir gefallen hat, zu einer davon wird. Live braucht überhaupt keine Dateien; ohne geladene Dateien verläuft die Karte quer von dunkel nach hell und nach oben von tonal nach geräuschhaft.

## Parameter

**Mute** Schaltet den Ausgang stumm, während die Wolke weiterfliegt, sodass du beim Aufheben der Stummschaltung mitten in der Textur wieder einsteigst.

**Record** Das rote Rec-Pad. Nimmt den Audio-Eingang in eine Audiodatei unter Dokumente auf und legt sie beim Stoppen in den ersten leeren Slot. Stille Takes werden verworfen, und ein Take ist auf zwei Minuten begrenzt.

**Live** Granuliert den Audio-Eingang, während er ankommt, und ist zu Beginn eingeschaltet. Die letzten acht Sekunden bleiben auf der Karte; älterer Klang fällt heraus, und Stille wird ausgelassen. Schalte es aus, um nur die geladenen Dateien zu spielen.

**Freeze** Hält den Live-Puffer fest, sodass die Wolke weiterspielt, was sie eingefangen hat, während der Eingang ignoriert wird. Nur mit Live an.

**X** Die horizontale Position des Cursors auf der Karte, 0 bis 1.

**Y** Die vertikale Position des Cursors auf der Karte, 0 bis 1.

**Density** Grains pro Sekunde.

**Scatter** Wie ungleichmäßig die Grains kommen. Bei null kommen sie genau im Abstand von Density, wie ein Uhrwerk; aufgedreht wird jede Lücke zufällig zwischen fast nichts und knapp doppelt so lang gewählt, während die mittlere Rate bleibt, wo Density sie hinsetzt, sodass die Wolke stolpert und klumpt, statt zu ticken.

**GrainSize** Grain-Länge in Millisekunden. Öffne die Spanne, und jedes Grain nimmt sich seine eigene Länge daraus.

**Fit** Skaliert das Pad auf den Bereich, in dem die Klänge tatsächlich liegen. Eine Bank, deren Punkte in einer Ecke landen, füllt das Pad, statt sich darin zu verstecken, und die Zahlen an den Rändern zeigen den Ausschnitt statt des ganzen Raums. Es zoomt nie näher als auf ein Viertel des Raums, sodass ein einzelner enger Cluster nicht zum Mikroskop wird, und es passt sich nur neu an, wenn sich die Punkte ändern - nie unter deiner Hand mitten im Ziehen. Aus zeigt den ganzen Raum, wie bisher.

**Spray** Auswahlradius um den Cursor. Klein bleibt auf einer Textur; groß greift aus der ganzen Nachbarschaft.

**Spread** Wie weit jedes Grain zufällig aus der Mitte gepannt wird.

**Pitch** Transposition in Halbtönen. Geschlossen ist es ein einziges Intervall; öffne es, und jedes Grain wird irgendwo aus der Spanne gezogen, sodass eine nur nach oben reichende Spanne ein steigendes Schimmern ergibt statt einer symmetrischen Unschärfe.

**Skew** Wo die Spitze innerhalb eines Grains sitzt, eine Spanne wie die anderen. Geschlossen bei null ist es eine glatte Glockenkurve; Richtung eins wird der Anstieg kürzer, bis das Grain angeschlagen wird und ausklingt, was aus einer Fläche einen Rhythmus macht, selbst bei gleicher Dichte.

**Shape** Wie viel eines Grains zwischen Anstieg und Abfall auf vollem Pegel offen gehalten wird. Bei null ist das Grain nur Anstieg und Abfall; Richtung eins wird es ein Block mit flachem Dach, was aus demselben Material eine glattere, durchgehendere Textur macht.

**Input** Pegel des Eingangs, wenn er in den Live-Puffer geht, bis zum Anderthalbfachen. Senk ihn, damit Feedback übernimmt, oder heb ihn, damit neues Spiel obenauf bleibt. Nur mit Live an.

**Feedback** Wie viel der Wolke zurück in den Live-Puffer geschrieben wird. Voll aufgedreht nährt sich die Wolke für immer selbst und verklingt nie; das Zurückschreiben ist sanft begrenzt, sodass es dichter wird und sättigt, statt außer Kontrolle zu geraten. Mit Pitch abseits von null landet jeder Durchlauf ein Intervall höher oder tiefer, sodass sich eine gehaltene Note zu einem Akkord stapelt. Nur mit Live an und nicht im eingefrorenen Zustand.

**Mix** Balance zwischen dem Direktsignal am Eingang und der Wolke. Standardmäßig nur Effektsignal.

**Quant** Wie stark ein Grain auf das von Sync gewählte Raster gezogen wird. Bei null gar nicht, voll aufgedreht wartet es auf die Rasterlinie. Wirkt nicht, solange Sync auf Free steht.

**Decorrelation** Wie sehr die Zufallsentscheidungen eines Grains übereinstimmen. Bei null entscheidet ein einziger Wurf über Tonhöhe, Länge, Hold und Panning zusammen, sodass höhere Grains auf einer Seite landen; voll aufgedreht wird jedes davon einzeln gewürfelt.

**Voices** Höchstzahl der Grains, die gleichzeitig klingen dürfen, 1 bis 200. Weniger dünnt die Textur aus und kostet weniger; mehr macht sie dichter.

**Sync** Ein Drehschalter, der jedes Grain am Raster des Transports festhält: Free, eine Zweiunddreißigstel bis eine Viertelnote oder ein Takt. Density entscheidet weiterhin, wie oft ein Grain fällig ist; Sync lässt es auf die nächste Rasterlinie warten. Nur während der Transport läuft.

**Gain** Ausgangspegel.

**File1** Die erste Korpus-Datei, und so weiter für File2 bis File4. Die Analyse läuft, sobald eine Datei gewählt wird.

## Rezept

**Wandernde Fläche** Lade eine Stimmaufnahme in File1 und ein Field Recording in File2. Density 12, GrainSize 200, Spray 0.08, Spread 0.8, Pitch ein wenig zu beiden Seiten geöffnet. Leg ein Kabel von einem LFO mit 0.02 Hz auf X und von einem zweiten mit 0.03 Hz auf Y, damit der Cursor über die Karte wandert, und schick den Ausgang durch ein Verbatim.

**Live-Wolke** Keine Dateien, Live an, eine Stimme oder ein Instrument am Eingang. Density 20, GrainSize 150, Spray 0.3, Pitch ein klein wenig geöffnet. Spiel eine Phrase, drück Freeze und lass den Cursor dann durch das fliegen, was du gerade gespielt hast. Für Akkorde heb Freeze auf, stell Pitch auf 7 und dreh Feedback auf 0.6.

## Verwandte Organisms

CameraIn, Sampler
