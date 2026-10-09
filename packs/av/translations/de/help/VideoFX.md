# VideoFX

Eine Bildbearbeitung mit Reglern für Position, Größe, Drehung, Farbe und Mosaik.

Ein Videoeingang und ein Ausgang. Das Bild läuft durch eine Transformationsstufe (Position, Größe, Drehung, Spiegelung und Verpixelung) und dann durch eine Farbstufe (Helligkeit, Kontrast, Sättigung, Farbton und Invertierung). Jeder Regler ist ein gewöhnlicher Parameter, also schiebt ein Follower auf Scale das Bild mit dem Kick, ein LFO auf Rotate dreht es langsam, und ein Sequence auf Hue lässt die Farbe im Takt kreisen. Setz es zwischen einen VideoPlayer, ein VideoPad oder CameraIn und ein Lumen, VideoMix oder VideoOut, und häng zwei hintereinander, wenn Transformation und Farbe getrennt automatisiert werden sollen.

## Parameter

**PosX** Wo das Bild von links nach rechts sitzt, als Anteil des Frames. +100% schiebt es einen ganzen Frame nach rechts.

**PosY** Dasselbe nach oben und unten. +100% ist ein ganzer Frame nach oben.

**Scale** Wie groß. 100% ist wie geliefert, 200% doppelt so groß, 0% schrumpft es auf einen Punkt.

**Rotate** Die Drehung in Grad um die Mitte des Frames.

**Pixelate** Wie grob das Mosaik ist. 0% lässt das Bild in Ruhe; 100% sind nur eine Handvoll Blöcke über die Breite.

**Mirror** Faltet den Frame an seiner Mitte. Off lässt ihn ganz, Left kopiert die linke Hälfte auf die rechte, Top kopiert die obere Hälfte auf die untere, Both macht beides.

**Brightness** Das Licht. 100% ist wie geliefert, 0% ist schwarz, 200% doppelt so hell.

**Contrast** Die Spreizung um mittleres Grau. 0% macht alles flach grau, 200% treibt es in die Extreme.

**Saturation** Der Regler Colour. 0% ist monochrom, 100% wie geliefert, 200% übersteuert.

**Hue** Dreht jede Farbe um den Farbkreis, in Grad.

**Invert** Zeigt das Negativ.

## Rezept

**Kick-Zoom** Verbinde einen VideoPlayer mit VideoFX und VideoFX mit einem VideoOut. Schick den Kick in einen Follower mit kurzem Release und leg den Follower mit geringer Tiefe über 100% auf Scale, sodass jeder Schlag das Bild auf dich zuschiebt und es sich zwischen den Schlägen wieder setzt. Etwas Pixelate dazu gibt eine raue Kante.

## Verwandte Organisms

VideoPlayer, VideoPad, VideoMix, Lumen
