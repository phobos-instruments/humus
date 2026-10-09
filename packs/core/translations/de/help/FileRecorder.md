# FileRecorder

Ein Durchgangs-Organism, das alles, was bei ihm ankommt, in eine Audiodatei schreibt, während das Patch läuft.

Der Eingang geht unverändert zum Ausgang, also kannst du es überall im Patch in ein Kabel setzen und dort einen Stem aufnehmen, nicht nur am Master. Das Dropdown Tracks legt fest, wie viele Stereodateien es schreibt, 1 bis 32, mit einer Dateizeile pro Spur; die Kanäle werden der Reihe nach auf die Dateien verteilt, wobei der Ch-Spinner jeder Zeile sagt, wie viele sie nimmt, und eine Datei, die über den letzten Eingang hinausreicht, nimmt Stille auf. Die Dateien werden abseits des Audiopfads geschrieben, sodass eine langsame Festplatte den Klang nicht unterbrechen kann, und jede Zeile zeigt den Pegel dessen, was ankommt, ob gerade ein Take läuft oder nicht. Aufnehmen im Transport nimmt dagegen scharf geschaltete AudioTrack-Organisms auf die Timeline auf; nimm dieses Organism, wenn du einfache Dateien auf der Festplatte willst.

## Parameter

**File_1** Wohin Datei 1 geht, und so weiter für die Zeilen darunter. Lass es leer, und jedes Scharfschalten schreibt einen Take mit Zeitstempel in den Aufnahmeordner und trägt ihn in die Zeile ein; eine Datei, die du selbst benannt hast, bleibt und wird wiederverwendet.

**RequestedChannelCounts_1** Wie viele Kanäle Datei 1 nimmt, und so weiter für die Zeilen darunter. Standardmäßig zwei; null schaltet die Zeile ab.

**FileMode** Overwrite ersetzt die benannte Datei. Append nimmt hinter ihrem Ende weiter auf, sodass sich eine Aufnahme über mehrere Durchgänge aufbauen lässt.

**PunchMode** Manual und SoundIn Sync stoppen über den Knopf. Timed stoppt nach RecordDuration von selbst und schließt die Datei ab.

**RecordDuration** Wie lange ein Timed-Durchgang läuft, in Millisekunden.

**Record** Schaltet alle Dateien auf einmal scharf und wieder ab. Es schaltet sich selbst wieder aus, wenn keine Zeile Kanäle zum Schreiben hat, und ein Take muss hier gestoppt und nicht einfach liegen gelassen werden, denn erst das Stoppen schreibt den Header, der die Datei lesbar macht.

## Rezept

**Drum-Stem** Setz den FileRecorder zwischen das Drum-Organism und den Mixer, lass File_1 leer, Ch auf 2, PunchMode Manual und FileMode Overwrite. Schalte Record für den Durchgang scharf und am Ende wieder ab; die Zeile zeigt, wo der Take gelandet ist, und der Mix lief unverändert weiter, während er geschrieben wurde.

## Verwandte Organisms

AudioTrack, SoundOut, Mixer, Bus
