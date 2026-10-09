# Decomposer

Audio rein, MIDI-Noten raus: ein Pitch-Tracker, der das Gespielte schon beim Spielen in Noten verwandelt, einstimmig oder als ganzen Akkord.

Stell Voices auf 1, und es folgt einer Stimme; das ist die beste Einstellung für eine Bassline, eine gesungene Melodie, ein Lead oder eine Gitarre, die Ton für Ton gespielt wird. Die Tonhöhensuche bleibt auf den Bereich von LowNote bis HighNote beschränkt, das ist sein bester Schutz gegen Oktavfehler. Aussetzer über einen einzelnen Frame werden geglättet, und eine wiederholte Note wird ab ihrem Einsatz neu angeschlagen, statt zu einer einzigen gehaltenen Note zu verschmelzen. Es meldet, was es hört, ohne Korrektur auf Tonart oder Tonleiter; damit das Ergebnis sauber in der Tonart liegt, schick die Noten durch ein Trellis. Die Anzeige im Editor zeigt die gesendete Note und wie weit die Quelle zu hoch oder zu tief liegt. Verbinde ein SoundIn mit seinem Eingang und seinen MIDI-Ausgang mit einem beliebigen Instrument, oder mit einem MidiMonitor, um zuzuschauen. Rechne mit ein paar Dutzend Millisekunden Latenz beim Erkennen.

## Parameter

**Voices** Wie viele Noten gleichzeitig. Bei 1 folgt es einer einzelnen Linie, mit der schnellsten Reaktion und dem stabilsten Tracking. Über 1 hört es auf einen Akkord und sendet jede Note, die es findet, bis zu sechs.

Ab 2 arbeitet es anders: Statt einer Tonhöhe zu folgen, nimmt es den Klang auseinander. Es bewertet jede Note, die es hören könnte, nach den Teiltönen, die diese Note erzeugen würde, nimmt die stärkste, entfernt, was diese Note erklärt, und sucht im Rest erneut. So kann es einen Akkord von einer einzelnen Note mit reichem Klang unterscheiden. Das kostet Zeit: Für einen Akkord braucht es etwa eine Drittelsekunde Klang, um sicher zu sein, gegenüber ein paar Dutzend Millisekunden für eine einzelne Linie. Für alles, was Ton für Ton gespielt wird, bleibt Voices 1 deshalb die richtige Einstellung.

Stell so viele Noten ein, wie du spielen willst, nicht mehr. Sechs bei einem Dreiklang finden keine drei zusätzlichen Noten, geben ihm aber drei weitere Gelegenheiten, sich zu irren. Am sichersten ist es bei Klängen mit klarer Obertonreihe und gleichmäßigem Pegel: ein Klavier, eine cleane Gitarre, eine Orgel, ein Synth-Pad. Am unsichersten ist es in der untersten Oktave, wo die Noten zu dicht beieinanderliegen, um sie schnell zu trennen, und bei allem Perkussiven oder Hauchigen, wo es keine Obertonreihe zum Zerlegen gibt. Sensitivity legt fest, wie leise eine Note im Verhältnis zur lautesten im Akkord sein darf, bevor sie weggelassen wird.

**Sensitivity** Senkt die Schwellen für Pegel und Klarheit. Erhöh den Wert für leise oder hauchige Quellen, senk ihn, wenn Rauschen verirrte Noten auslöst.

**Response** Wie viele Frames eine Tonhöhe halten muss, bevor es sich festlegt. Fast reagiert schneller, Accurate ist bei verrauschten Quellen stabiler, Balanced liegt dazwischen.

**LowNote** Die tiefste Note, die es meldet. Heb sie auf den echten Umfang deiner Quelle an, damit Rumpeln darunter nicht gewählt werden kann.

**HighNote** Die höchste Note, die es meldet. Senk sie auf den echten Umfang deiner Quelle, damit Oktavfehler darüber nicht gewählt werden können.

**Channel** Der MIDI-Kanal, auf dem die Noten rausgehen.

## Rezept

**Akkord aufs Pad** Verbinde ein SoundIn von einer Gitarre oder einem Keyboard mit dem Decomposer, Voices 4, LowNote 40, HighNote 84, und seinen Ausgang mit einem Rhizome oder einem Sampler. Spiel einen Akkord, und die ganze Form kommt auf der anderen Seite heraus. Die Anzeige zeigt die Noten, die es sendet, die tiefste zuerst, und der Klangbalken daneben zeigt, wie harmonisch ist, was es hört: Ein voller Balken ist ein sauberer Akkord, ein leerer ist Rauschen, das es nicht benennt.

**Stimme zum Synth** Verbinde ein SoundIn von einem Mikrofon mit dem Decomposer und seinen Ausgang mit einem Rhizome. Response Balanced, Sensitivity 0.5, LowNote 48, HighNote 84 für einen typischen Gesangsumfang. Summ eine Linie, und der Synth spielt sie mit; setz ein Trellis zwischen die beiden, damit sie in der Tonart bleibt.

## Verwandte Organisms

Trellis, OscMonitor, MidiMonitor
