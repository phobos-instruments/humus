# Board

Ein Mikrocontroller-Board an einem seriellen Kabel: Seine analogen Eingänge kommen als Steuerausgänge heraus, und seine Buchsen steuern Pins auf dem Board.

Flash das Board einmal mit dem Standard-Sketch zur Pin-Steuerung, der als Beispiel beim eigenen Editor des Boards mitkommt, steck es ein und wähle ein Preset für dein Board. Jeder Ausgang liest einen analogen Kanal als 0 bis 1, egal welche Auflösung das Board hat, also wird ein Potentiometer oder ein Lichtsensor an A0 zu Ausgang 1: Verbinde ihn mit einem Number, um ihn zu beobachten, oder mit der Buchse eines Gains, um einen Pegel zu formen. Jede Buchse steuert einen Pin und schaltet oder dimmt ihn. Das Board wird abseits des Audiopfads gelesen, und nach erneutem Einstecken verbindet es sich von selbst wieder. Board ist auch eine Steuerquelle, also bietet das Rechtsklickmenü jedes Parameters Steuern mit Board an. Ein Board ohne den Standard-Sketch bekommt drei Sekunden zum Booten und wird dann trotzdem angesprochen; ein Board mit einem eigenen Sketch gehört stattdessen an SerialIn und SerialOut.

## Die Anzeige

Die Zeile unter den Ausgängen zeigt, wo die Box gerade steht: sucht ein Gerät, wartet auf den Port, bootet, spricht mit dem Board (talking), Pins zugeordnet. Die Ausgänge bleiben auf null, bis das Board hochgefahren ist. Die Presets legen fest, welche Kanäle die Ausgänge lesen und welche Pins die Buchsen steuern; das Board meldet dann selbst, was jeder Pin kann, also funktioniert auch ein Board, das nicht auf der Liste steht, mit von Hand eingetippten Pins.

## Parameter

**Device** An welchem seriellen Gerät das Board hängt. Auto nimmt das erste gefundene USB-Seriell-Gerät; der Rest der Liste sind alle vorhandenen Geräte. Beliebig viele Board-, SerialIn- und SerialOut-Boxen können sich einen Port teilen.

**BaudRate** Die Geschwindigkeit des Ports. Der Standard-Sketch öffnet mit 57600, dem Standardwert hier; ändere ihn nur, wenn du den Sketch geändert hast.

**Custom** Jede Rate, die nicht auf der Liste steht. Null überlässt BaudRate die Wahl.

**Reset** An: Beim Öffnen des Ports startet das Board neu, so wie es sein Editor tut, und genau das erwartet der Standard-Sketch. Aus: Das Board läuft über ein Neuverbinden hinweg weiter.

**Reconnect** Schließt den Port und öffnet ihn neu, für jede Box, die ihn teilt. Nutze es, wenn ein Board nicht mehr antwortet.

**Rate** Wie oft das Board seine analogen Eingänge abtastet und sendet, in Millisekunden. 19 ist der eigene Standard des Sketches; niedriger ist lebendiger und belastet die Leitung mehr.

**Outlets** Wie viele Ausgänge die Box zeigt, eins bis sechs.

**Inlets** Wie viele Buchsen die Box zeigt, eins bis sechs.

**In1** Die erste Buchse und der Pegel, den ihr Pin bekommt, wenn kein Kabel daran hängt. Entsprechend für 2 bis 6.

**Analog1** Der analoge Kanal, den Ausgang 1 liest, 0 bis 15. Ab Werk liest Ausgang 1 A0, Ausgang 2 A1 und so weiter für 2 bis 6.

**Pin1** Der Board-Pin, den Buchse 1 steuert. Entsprechend für 2 bis 6.

**Mode1** Digital setzt den Pin auf high, wenn die Buchse über 0.5 liegt, sonst auf low. PWM setzt das Tastverhältnis des Pins aus den 0 bis 1 der Buchse; nur manche Pins können PWM, und die Presets legen diese auf die Buchsen 2 bis 6. Entsprechend für 2 bis 6.

**Port** Ein expliziter Gerätepfad, der Device ersetzt, solange er gesetzt ist. Bleibt normalerweise leer.

## Rezept

**Sensor auf Filter** Schließ ein Potentiometer an A0 an, wähle das Preset deines Boards und prüf, ob die Anzeige talking meldet. Rechtsklicke auf Frequency eines Filters, wähle Steuern mit Board out1, und der Regler folgt dem Poti. Verbinde den env-Ausgang eines Followers mit In2, stell Mode2 auf PWM, und die LED an diesem Pin dimmt mit der Musik.

## Verwandte Organisms

SerialIn, SerialOut, Number, Follower
