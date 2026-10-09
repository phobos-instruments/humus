# Firefly

Ein Blitz über dem Bild, auf den Schlag, von Hand oder durch Noten.

Ein Video-Eingang und ein Ausgang: Was hereinkommt, läuft durch, und der Blitz wird darübergelegt. Ist nichts angeschlossen, landet der Blitz auf Schwarz, Firefly allein ist also eine Lampe. Es blitzt auf dem Raster des Transports, in einer eigenen freien Rate, über den Flash-Knopf oder durch Noten, die an seinem MIDI-Eingang ankommen, sodass ein MIDI Track, DNA oder Riff auf der Timeline es wie alles andere steuert. Die Velocity der Note bestimmt, wie stark dieser Blitz ist. Hue und Saturation geben ihm eine Farbe, und weil beide gewöhnliche Parameter sind, lässt ein LFO auf Hue die Farbe wandern, während du spielst. Jeder Blitz wird von der eigenen Uhr des Bildes getaktet, sodass auch ein Blitz, der kürzer als ein Frame ist, noch zu sehen ist. Eine Warnung, die du ernst nehmen solltest: Schnelles Blitzen kann bei Menschen mit fotosensitiver Epilepsie Anfälle auslösen. Lass es vor einem Publikum, das du nicht kennst, nur langsam blitzen, und kündige es an.

## Parameter

**Mode** Was den Blitz von selbst auslöst. Beat blitzt auf dem Raster des Transports und bleibt beim Song; bei gestopptem Transport ruht es. Free läuft mit seiner eigenen Rate, ob der Song spielt oder nicht. Played blitzt nie von selbst: Nur der Flash-Knopf und Noten am MIDI-Eingang lösen aus, sodass die Lichter deinen Händen oder einem MIDI Track folgen und sonst nichts. Knopf und Noten lösen in jedem Modus aus.

**Division** Wie oft der Beat-Modus im Verhältnis zum Schlag blitzt: von einer ganzen Note bis zu einer Zweiunddreißigstel, dazu Triolen und punktierte Werte.

**Rate** Blitze pro Sekunde im Free-Modus, von einem langsamen Puls bei 0.05 Hz bis 30 Hz.

**Width** Wie lange jeder Blitz dauert, als Anteil der Lücke zwischen den Blitzen; im Played-Modus ist die Lücke ein Schlag im Tempo des Songs. Kurz ist ein Kamerablitz; breit ist ebenso lange hell wie dunkel.

**Hue** Die Farbe des Blitzes in Grad auf dem Farbkreis. Sie zeigt sich nur, wenn Saturation aufgedreht ist.

**Saturation** Wie farbig der Blitz ist. 0% ist Weiß, 100% der volle Farbton.

**Level** Wie stark der Blitz das Bild überdeckt. 100% ersetzt es für die Dauer des Blitzes.

**Flash** Löst einen Blitz von Hand aus. Weise ihn einem Pad oder einem Fußschalter zu und spiel die Lichter.

## Rezept

**Beat-Blitz über einem Video** Verbinde einen VideoPlayer mit Firefly und Firefly mit einem VideoOut. Mode Beat, Division 1/4, Width 12%, Shape 40%, Saturation 0%. Drück auf Wiedergabe, und das Bild blitzt auf jedem Schlag auf.

**Von der Timeline gespielt** Stell Mode auf Played, verbinde einen MIDI Track mit dem MIDI-Eingang von Firefly und schreib eine Notenfolge. Laute Noten blitzen hart, leise sind kaum zu sehen, sodass die Lichter den Part lesen, den du geschrieben hast.

## Verwandte Organisms

VideoFX, Lumen, VideoOut
