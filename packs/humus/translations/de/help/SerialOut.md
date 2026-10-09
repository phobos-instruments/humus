# SerialOut

Schickt Steuerwerte über einen seriellen Port hinaus, in welcher Text- oder Byteform das Gerät sie auch erwartet.

Leg Slider, einen Follower, einen LFO oder ein Number auf seine Buchsen und schreib die Zeile, die daraus werden soll, ins Feld Format: %1 bis %8 geben die Buchsen als Text aus, %b1 bis %b8 schicken eine als rohes Byte und %w1 bis %w8 als Zwei-Byte-Wort, \n beendet eine Zeile und \xNN ist ein rohes Byte. Der Standard "%1 %2\n" gibt zwei Zahlen und einen Zeilenumbruch aus. Scale multipliziert einen Wert, bevor er ausgegeben wird, und Int rundet ihn, sodass ein Steuerwert von 0 bis 1 als 0 bis 255 hinausgeht. Mehrere Zeilen in Format sind mehrere Nachrichten, und eine Message-Buchse wählt, welche hinausgeht. Die Anzeige unten zeigt den Port und die zuletzt gesendeten Bytes. SerialIn liest Werte auf dieselbe Weise zurück.

## Parameter

**Device** Mit welchem seriellen Port gesprochen wird. Auto nimmt den ersten gefundenen Port; der Rest der Liste sind alle vorhandenen Ports.

**BaudRate** Die Port-Geschwindigkeit, die zu der passen muss, die die Firmware öffnet. 9600 und 115200 sind die üblichen Werte.

**Rate** Wie oft pro Sekunde die Zeile im Modus Continuous hinausgeht, und die Obergrenze in den beiden anderen. Angezeigt als Per second.

**Frame** Datenbits, Parität und Stoppbits: 8N1 nutzt fast alles; 8E1, 8O1 und 8N2 sind für die Geräte da, die sie verlangen.

**Custom** Jede andere Baudrate, eingetippt. 0 überlässt der BaudRate-Liste die Wahl.

**Reset** Pulst die Reset-Leitung des Boards, wenn der Port öffnet, sodass sein Programm bei jedem Verbinden von vorn startet. Aus lässt das Board über ein erneutes Verbinden hinweg weiterlaufen.

**Reconnect** Schließt den Port und öffnet ihn wieder, für jedes Organism, das ihn teilt.

**Send** Continuous schreibt die Zeile Rate-mal pro Sekunde. On change schreibt sie einmal, sobald sich eine der angezeigten Buchsen bewegt. On trigger schreibt sie, wenn die Trigger-Buchse steigt.

**Values** Wie viele Buchsen das Organism zeigt und wie viele für On change zählen. Angezeigt als Sockets.

**Value1** Die erste Buchse und die Zahl, die sie sendet, wenn kein Kabel darauf liegt. Und so weiter für 2 bis 8; %5 in Format gibt Value5 aus, ob seine Buchse angezeigt wird oder nicht.

**Trigger** Die Buchse, die On trigger liest; ein Button darauf schickt die Nachricht einmal pro Druck.

**Message** Wählt, welche Zeile eines mehrzeiligen Format hinausgeht, ab 1. Sie erscheint, sobald Format mehr als eine Zeile hat.

**Scale1** Multipliziert Value1, bevor es ausgegeben oder in ein Byte gepackt wird. Und so weiter für 2 bis 8.

**Int1** Gibt Value1 als ganze Zahl statt mit Nachkommastellen aus. Und so weiter für 2 bis 8.

**Format** Die Nachrichtenvorlage. Neben den Platzhaltern oben gibt %n1 den Namen der Quelle an einer Buchse aus, %c ein Zähl-Byte, %x und %X eine Prüfsumme und %(v1*180) einen berechneten Wert.

**Port** Ein von Hand eingetippter Portpfad, der die Wahl bei Device überschreibt, solange er gesetzt ist.

## Rezept

**Servo von einem LFO** Leg einen LFO auf Value1, setz Scale1 auf 180 und Int1 an, Format "M %1\r", Send Continuous, Rate 30. Das Board liest die Zeile und bewegt das Servo mit dem LFO; verlangsame den LFO auf einen Takt, und der Arm schwenkt im Takt des Patches.

## Verwandte Organisms

SerialIn, Number, Slider, Button
