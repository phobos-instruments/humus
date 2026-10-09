# Helix

Ein Live-Looper mit vier Strands und Rec, Dub, Play und Stop auf jedem Strand.

Jeder Strand ist ein eigener Loop von bis zu 30 Sekunden. Drück Rec zum Aufnehmen und noch einmal Rec, um den Loop zu schließen und abzuspielen, oder Dub, um ihn zu schließen und gleich weiter zu schichten. Dub startet und beendet einen Overdub auf einem laufenden Loop, und hältst du es gedrückt, wird nur so lange overdubbt, wie du hältst. Auch Play und Stop schließen eine Aufnahme, spielend oder still. Jeder Overdub ist eine Schicht, die Undo wieder abzieht und Redo zurückholt. Jeder Strand hat seinen eigenen Sync: Free schließt den Loop genau dort, wo du drückst, Beat und Bar rasten Tastendrücke, Rev und Half auf das Raster des Transports ein, und ein etwas zu später Druck wird auf die Rasterlinie zurückgesetzt, die du gemeint hast. Follow lässt sie statt am Transport an der Nahtstelle eines Loops einrasten, der schon auf einem anderen Strand läuft; so wird ein erster, frei von Hand gespielter Loop zum Raster, in das sich die anderen einfügen. Läuft sonst nichts, wartet es auf den Takt. Die Eingänge 1 und 2 sind der Haupteingang und speisen jeden Strand; die Eingänge 3 bis 10 sind ein Stereopaar pro Strand, und ein Strand nimmt sein eigenes Paar zusätzlich zum Haupteingang auf, sodass ein angeschlossenes Paar ihm eine Stimme gibt, die die anderen nicht hören. Der Hauptausgang führt die abgehörten Eingänge plus jeden hörbaren Strand, und die Ausgänge 3 bis 10 sind Stereo-Direktausgänge, ein Paar pro Strand, nach seinem Level, Stumm und Solo. Play all und Stop all steuern alle Strands auf einmal. Loops werden mit dem Patch als Audiodateien in seinem Projektordner gespeichert und öffnen sich gestoppt wieder, bereit für Rec oder Play. Um Loops auf die Timeline zu bringen, klick mit der rechten Maustaste auf das Organism und wähl Audio auf Spur: einen Strand oder Alle für jeden aufgenommenen Strand, jeden auf seiner eigenen Audiospur am Takt unter der Abspielposition. Du kannst einen Strand auch an der Mitte seines Rings herausziehen und auf der Timeline dort ablegen, wo du ihn haben willst. Schließ ein SoundIn am Eingang an und weise Rec1 eines Strands einem Fußschalter zu, um mit einem einzigen Pedal aufzunehmen, abzuspielen und zu overdubben.

Der Ring jedes Strands zeigt, was dort aufgenommen ist: die eigene Wellenform des Loops, hell hinter der Abspielposition und gedämpft davor, mit der Zahl der Schichten in der Mitte. Ein Lichtkranz außen um den Ring folgt dem Pegel, der an diesem Strand ankommt, so siehst du, wohin der Klang geht, bevor du etwas scharf schaltest. Die zwei Pfeile unter dem Ring rücken einen laufenden Loop zurecht: Halt den linken, und der Strand läuft drei Prozent langsamer, halt den rechten, und er läuft drei Prozent schneller, und wenn du loslässt, behält er die Stelle, die er erreicht hat. Ein kurzes Tippen rückt ihn also nur um Haaresbreite, langes Halten um ein großes Stück. Das ist die Lösung für einen Loop, der ein wenig vor oder hinter den anderen liegt. Die Tonhöhe verbiegt sich dabei leicht, wie bei einer Hand auf einer Schallplatte.

## Parameter

**Rec1** Der Ablauf mit nur einem Schalter, für einen Fußschalter oder ein Pad, nicht auf der Frontplatte: aufnehmen, schließen und abspielen, overdubben, abspielen. Halte es während der Wiedergabe, um nur so lange zu overdubben, wie du hältst. Dasselbe für die Strands 2 bis 4.

**Record1** Rec: startet die Aufnahme auf einem leeren Strand und schließt während der Aufnahme den Loop und spielt ihn ab.

**Dub1** Dub: startet einen Overdub auf einem laufenden oder gestoppten Loop und schließt ihn beim nächsten Druck ab; halte es, um nur so lange zu overdubben, wie du hältst. Während der Aufnahme schließt es den Loop und geht direkt in den Overdub.

**Play1** Startet den Strand vom Anfang seines Loops neu, auf dem Raster, wenn Sync es so will. Es startet auch einen Loop, den Stop still geschlossen hat, und während der Aufnahme schließt es den Loop und spielt ihn ab.

**Stop1** Hält den Strand an. Während der Aufnahme schließt es den Loop still, bereit zum Starten.

**Undo1** Zieht die neueste Schicht ab. Während der Aufnahme verwirft es den Take.

**Redo1** Legt die zuletzt entfernte Schicht zurück.

**Clear1** Leert den Strand. Auf der Frontplatte musst du es einen Moment halten, damit ein versehentlicher Klick keinen Loop kosten kann.

**Level1** Der Pegel des Strands im Hauptmix und an seinem Direktausgang.

**Mute1** M: schaltet den Strand stumm. Er läuft darunter weiter und kommt deshalb phasengleich zurück.

**Solo1** S: nur dieser Strand ist zu hören.

**Sync1** Free, Beat, Bar oder Follow: worauf Tastendrücke, Rev und Half einrasten. Follow nutzt die Nahtstelle des spielenden Strands mit der niedrigsten Nummer, sodass sein Loop zum Raster wird; leerst du diesen Strand, übernimmt der nächste spielende. Ein neues Helix startet Strand 1 auf Bar und die Strands 2 bis 4 auf Follow, damit der erste Loop die Länge festlegt, an die sich die anderen koppeln.

**Rev1** Spielt den Strand rückwärts. Umschalten beendet einen offenen Overdub.

**Slice1** Welches Viertel des Strand-Loops gehalten wird, 0 für keines. Klick auf ein Viertel des Rings, um es zu halten, und lass los, um es freizugeben, so wie die vier Pads auf einem Slicing-Pad arbeiten; der Parameter lässt sich wie jeder andere einem Fußschalter oder einem Pad zuweisen.

**Nudge1** -1, solange der linke Pfeil gehalten wird, 1, solange der rechte gehalten wird, sonst 0. Ein gehaltenes Nudge lässt einen laufenden Strand drei Prozent langsamer oder schneller laufen, und der Strand behält die erreichte Stelle; auf einen Strand, der aufnimmt, overdubbt oder gestoppt ist, wirkt es nicht. Es lässt sich wie jeder andere Parameter zuweisen, sodass ein Paar Fußschalter oder ein einzelnes Rad mit Mittenrückstellung einen Loop ohne Maus zurechtrückt.


**Half1** Spielt den Strand mit halber Geschwindigkeit, eine Oktave tiefer. Eine Schicht, die bei halber Geschwindigkeit overdubbt wurde, spielt mit doppelter Geschwindigkeit, wenn du es wieder ausschaltest.

**Shot1** Once: Der Strand spielt einen einzigen Durchgang und stoppt. Rec startet ihn erneut.

**Decay** Wie viel von den älteren Schichten jeden Overdub-Durchgang überlebt. Unter 1 entwickelt sich ein Loop immer weiter, statt sich aufzutürmen.

**Monitor1** Off, Mix oder Out: wo der Live-Eingang des Strands, also der Haupteingang plus sein eigenes Paar, zu hören ist. Mix spielt ihn über den Hauptausgang, und der Haupteingang ist nur einmal zu hören, egal wie viele Strands Mix wählen; Out spielt ihn auf dem eigenen Direktausgang des Strands, für einen Strand, der auf seinen eigenen Kanal läuft. Dasselbe für Monitor2 bis Monitor4.

**Monitor** Die eine Monitor-Einstellung älterer Patches: Ein Strand ohne eigenes Monitor folgt ihr.

**Follow** Friert jeden Strand stumm ein, solange der Transport steht, und macht weiter, sobald er läuft, synchronisierte Strands wieder auf dem Takt. Ist es aus, laufen Loops trotzdem weiter. Eine Aufnahme wird nie unterbrochen.

**PlayAll** Play all: startet jeden aufgenommenen Strand vom Anfang seines Loops neu, alle zusammen, auf dem gröbsten Raster, das einer von ihnen nutzt: auf dem Takt, wenn einer auf Bar steht, auf dem Schlag, wenn einer auf Beat steht. Strands, die schon spielen, starten ebenfalls neu, sodass auseinandergedriftete Loops wieder in Phase kommen. Ein Strand auf Once, der fertig ist, wartet auf sein eigenes Rec oder Play.

**StopAll** Stop all: hält jeden laufenden Strand an, und ein Strand, der noch aufnimmt, schließt seinen Loop still. Es wartet nur dann auf den Takt, wenn jeder laufende Strand auf Bar steht, und auf den Schlag, wenn keiner auf Free steht; ein einziger Strand auf Free lässt es alles sofort beim Drücken stoppen.

**Loop1** Die gespeicherte Audiodatei des Strands, beim Speichern neben das Patch geschrieben, mit dem Tempo, in dem sie aufgenommen wurde. Dasselbe für Loop2 bis Loop4.

**Tempo1** Pitch, Stretch oder Off: was der Strand tut, wenn sich das Tempo von dem entfernt, in dem er aufgenommen wurde. Pitch beschleunigt oder verlangsamt den Loop wie ein Band, samt Tonhöhe. Stretch ändert nur seine Länge und behält seine Tonhöhe, und ein Overdub behält die Tonhöhe, in der du ihn gespielt hast; jenseits des Achtfachen des Aufnahmetempos spielt er wie Pitch. Off ignoriert das Tempo. Dasselbe für Tempo2 bis Tempo4.

## Rezept

**Fester Teppich, freie Stimme** Sync1 Bar, Sync2 Free, Follow transport an, jedes Monitor auf Mix, Decay 0.85. Bei laufendem Transport und angeschlossenem SoundIn drückst du auf dem ersten Strand auf der Eins Rec, spielst vier Takte und drückst auf der nächsten Eins Dub, um weiter zu schichten, dann noch einmal Dub zum Abschließen. Drück mitten in einer Phrase Rec auf dem zweiten Strand, für eine Stimme, die gegen das Raster driftet. Schick die Ausgänge 3 und 4 in ein Fern, nur für den ersten Strand.

## Verwandte Organisms

Repeater, Leafcutter, Sampler
