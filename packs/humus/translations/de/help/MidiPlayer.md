# MidiPlayer

Ein Partitur-Player, der eine MIDI-Datei lädt, alle sechzehn ihrer Kanäle über seine eigenen Audioausgänge erklingen lässt und die Partitur über einen MIDI-Ausgang weitergibt, damit andere Instrumente sie ebenfalls spielen können.

Er liest Partiturdateien im Format .mid und .midi, Karaoke-Dateien im Format .kar und .mus, das kompakte Partiturformat der frühen Ego-Shooter, und braucht sonst keine Kabel: Wähle eine Datei und drücke Play. Die Stimmen sind standardmäßig Frequenzmodulation, eine kleine Bank aus Operator-Einstellungen in der Tradition von Heimcomputer und Soundkarte, und deshalb klingt er so, wie er klingt; zieh eine gesampelte Bank in den Bank-Slot, und er spielt stattdessen Aufnahmen. Mit Host tempo läuft die Partitur zum Transport, ausgeschaltet behält sie ihre eigene Tempo-Map. Verkable die Audioausgänge mit einem Fern oder einer Console. Der MIDI-Ausgang führt die Noten, Programme, Bends und Controller, die die Datei enthält, sodass ein daran angeschlossener Synth dieselbe Partitur mit seiner eigenen Stimme spielt; ein stummgeschalteter Kanal schweigt auch dort, und beim Stoppen werden alle Noten losgelassen. Für aufgenommenes Audio siehe FilePlayer, für eine Partitur, die von außen kommt, MidiIn.

## Parameter

**File** Die Partitur, die gespielt wird. Klicke auf den Slot, um eine auszuwählen, oder zieh eine Datei hinein.

**Bank** Die Instrumente, die die Kanäle spielen. Die mitgelieferte Chip-Bank im Format .wopn ist Frequenzmodulation; eine gesampelte Bank im Format .sf2 ersetzt die Chips durch Aufnahmen und ignoriert Chips.

**Play** Startet und stoppt die Partitur, genau wie die Play-Schaltfläche auf der Transportleiste; jede Schaltfläche auf dieser Leiste ist ein Parameter, also rechtsklicke auf eine beliebige für MIDI-Learn, OSC-Learn, Steuern mit oder Folgen. Stoppen bringt jede klingende Note zum Schweigen; drückst du Play, nachdem die Partitur zu Ende ist, beginnt sie von vorn. Er läuft auf seiner eigenen Clock, daher lässt ihn Stopp am Transport in Ruhe; drückst du Stopp ein zweites Mal, wenn nichts mehr läuft, hält er mit allem anderen an.

**Loop** Springt am Ende der Partitur zurück zum Anfang, statt zu verstummen.

**FollowHost** Host tempo: Ist es an, treibt das Tempo des Transports die Partitur; ist es aus, behält die Partitur das Tempo, in dem sie geschrieben wurde, samt der Tempowechsel, die der Komponist hineingeschrieben hat.

**Chips** Wie viele Soundchips gleichzeitig spielen, eins bis vier. Einer sind sechs Stimmen, und ein dichtes Arrangement verschluckt Noten; vier ergeben vierundzwanzig.

**Level** Ausgangspegel.

**Program1** Das Instrument, das Kanal 1 spielt. 0 spielt, was die Datei verlangt; jeder andere Wert überschreibt es für diesen Kanal. Dasselbe gilt für Program2 bis Program16. Kanal 10 ist der Drumkanal und bleibt Drums.

**Mute1** Schaltet Kanal 1 stumm, an den Audioausgängen wie am MIDI-Ausgang, und so weiter für Mute2 bis Mute16. Ein Kanal, auf dem die geladene Datei nie spielt, verblasst und reagiert nicht mehr, sodass alles, was noch leuchtet, tatsächlich in der Datei steckt.

## Rezept

**Klang von damals** Lade eine .mid, Host tempo aus, Chips 4, Loop an, Level 0.7, und drücke Play. Schalte den Kanal stumm, den du selbst spielen willst, und führe die Ausgänge durch ein Fern mit einem kurzen Slapback.

**Leih ihm eine Stimme** Lade eine Partitur, schalte den Kanal stumm, den du ersetzen willst, und verkable den MIDI-Ausgang mit einem Synth. Die Datei spielt wie geschrieben, nur eine Stimme singt mit deinem eigenen Klang.

## Verwandte Organisms

FilePlayer, MidiIn, pH
