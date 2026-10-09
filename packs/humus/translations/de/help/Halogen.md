# Halogen

Ein spektraler Player, der ein Bild in Klang verwandelt: Die Breite ist die Zeit, die Höhe die Tonhöhe, die Helligkeit die Lautstärke.

Ein Lesekopf liest jeweils eine Spalte des Bildes, und eine inverse Transformation macht daraus wieder Klang, du hörst also, was unter dem Kopf liegt. Zieh ein Bild auf die Fläche oder klick darauf, um eins auszuwählen; eine Datei, die kein Bild ist, wird als rohe Bytes in Zeilen gelesen, sodass auch eine Schriftdatei oder ein Archiv ein Spektrum hat. Ein Videokabel von einem VideoPlayer, einem CameraIn oder einem VideoTrack ersetzt das Bild Frame für Frame durch das bewegte Bild, und die Datei kehrt zurück, wenn das Kabel entfernt wird. Die Fläche zeigt das Bild so, wie der Kopf es hört, nach Gate, Blur und Tilt. Verbinde den Ausgang mit einem Fern oder einem Verbatim für Raum.

## Parameter

**File** Das Bild, das gelesen wird. Jede Datei geht: Bilder werden dekodiert, alles andere wird als Bytes gelesen.

**Scan** Sweep führt den Kopf mit Rate über das Fenster. Free parkt ihn bei X, sodass ein Regler, eine Hüllkurve oder Ziehen auf der Fläche ihn bewegt.

**Reverse** Läuft in die andere Richtung.

**Rate** Durchläufe pro Sekunde, 0.01 bis 100. Ganz oben wird der Durchlauf selbst zum Ton.

**X** Wo der Kopf in Free steht, oder wo der durchlaufene Bereich beginnt.

**Y** Die Unterkante des Lesebands.

**Width** Wie viel der Bildbreite durchlaufen wird.

**Height** Wie viel der Bildhöhe gelesen wird.

**Lowest** Wie die Unterkante des Bands klingt, in Hertz.

**Highest** Wie die Oberkante des Bands klingt, in Hertz. Die Zeilen sind logarithmisch zwischen beiden verteilt.

**Gate** Eine Helligkeitsschwelle. Pixel darunter sind stumm, damit ein dunkler Hintergrund nicht brummt.

**Blur** Macht das Bild weicher, sodass ein bewegter Kopf durch Kanten gleitet, statt zu springen.

**Tilt** Neigt die Balance zu den Tiefen oder zu den Höhen.

**Level** Ausgangspegel.

## Rezept

**Langsames Treiben** Lade ein Foto mit dunklem Hintergrund. Scan Sweep, Rate 0.1, Lowest 55, Highest 4000, Gate 0.15, Blur 0.4. Verbinde den Ausgang mit einem Verbatim und lass einen Durchlauf zehn Sekunden dauern; dann stell Scan auf Free und verbinde einen LFO mit X, um den Kopf zu bewegen.

## Verwandte Organisms

Prism, Wave, SoundSpace
