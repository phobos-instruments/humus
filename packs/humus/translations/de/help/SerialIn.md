# SerialIn

Liest Zahlen von einem seriellen Port und legt sie auf die Steuerausgänge a bis h.

Schließ ein Mikrocontroller-Board an, das Messwerte über seine serielle Verbindung ausgibt, und jede Zahl einer Zeile landet auf ihrem eigenen Ausgang: die erste auf a, die zweite auf b und so weiter. Trenne Zahlen mit Leerzeichen, Kommas oder Semikolons, und setze ein Wort vor eine Zahl, um sie ohne Kabel an die Var mit diesem Namen zu schicken. Eine Parse-Vorlage liest Zeilen oder Byte-Pakete, die keine reinen Zahlen sind, und solange eine gesetzt ist, pulst ein neunter Ausgang namens match, sobald eine Zeile passt. Der Port wird abseits des Audiopfads gelesen, ein wieder eingestecktes Gerät verbindet sich von selbst neu, und jeder Parameter kann in seinem Rechtsklickmenü Steuern mit SerialIn wählen. Verkable die Ausgänge mit den Buchsen von Number, Gain oder LFO.

## Parameter

**Device** Welcher serielle Port abgehört wird. Auto nimmt den ersten gefundenen Port; der Rest der Liste sind alle vorhandenen Ports.

**BaudRate** Die Portgeschwindigkeit, die zu dem passen muss, was die Firmware öffnet. 9600 und 115200 sind die üblichen Werte.

**Values** Wie viele der acht Ausgänge das Organism zeigt, ab a.

**Frame** Datenbits, Parität und Stoppbits: 8N1 verwendet fast alles; 8E1, 8O1 und 8N2 gibt es für die Geräte, die sie verlangen.

**Custom** Eine beliebige andere Baudrate, von Hand eingetippt. 0 überlässt der BaudRate-Liste die Wahl.

**Reset** Pulst beim Öffnen des Ports die Reset-Leitung des Boards, sodass sein Programm bei jeder Verbindung von vorn startet. Aus lässt das Board über eine Neuverbindung hinweg weiterlaufen.

**Reconnect** Schließt den Port und öffnet ihn wieder, für alle Organisms, die ihn teilen.

**Parse** Die Vorlage, zu der eine Zeile oder ein Paket passen muss: %1 bis %8 fangen Zahlen als Text ein, %b1 bis %b8 einzelne Bytes und %w1 bis %w8 Zwei-Byte-Wörter, beide gelesen als 0 bis 1. Leer nimmt jede Zahl der Zeile der Reihe nach; eine Zeile, die nicht passt, wird ignoriert und als skipped angezeigt.

**Port** Ein von Hand eingetippter Portpfad, der die Auswahl unter Device überschreibt, solange er gesetzt ist.

## Rezept

**Lichtsensor auf Filter** Lass das Board seinen Sensor dreißigmal pro Sekunde als Zahl von 0 bis 1 ausgeben, eine pro Zeile. Lass Parse leer, Values 1, BaudRate 115200. Rechtsklicke bei einem Filter auf Cutoff, wähle Steuern mit SerialIn a, und der Filter folgt dem Sensor. Beobachte die Anzeige unter Parse, um zu prüfen, was ankommt.

## Verwandte Organisms

SerialOut, Number, Follower, LFO
