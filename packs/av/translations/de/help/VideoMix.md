# VideoMix

Ein A/B-Crossfader für zwei Videoquellen oder ein Kanalmixer für bis zu acht.

Inputs legt fest, wie viele Quellen es annimmt. Bei 2 ist es ein Crossfader: Eingang 1 ist A, Eingang 2 ist B, und der Ausgang liefert die Überblendung. Fade zeigt bei 0 A, bei 1 B, und überall dazwischen mischt es die beiden nach dem Verlauf, den Curve festlegt. Fade ist ein ganz normaler Parameter: Weise ihm einen Controller zu, und du hast einen Hardware-Crossfader, leg einen synchronisierten LFO darauf, damit die beiden Quellen alle sechzehn Schläge die Plätze tauschen, oder zeichne ihn in eine Automationsspur, damit die Schnittfolge Teil des Songs wird. Blende vor dem VideoOut zwischen zwei kompletten Lumen-Composites über, oder wähle mehr Eingänge für einen Kanalmixer.

Ab 3 Eingängen wird es zum Kanalmixer mit einem senkrechten Fader pro Eingang. Die Kanäle liegen der Reihe nach übereinander, Kanal 1 ganz unten, und jeder Fader bestimmt, wie stark sein Kanal die darunterliegenden verdeckt: Ganz oben verdeckt er sie, in der Mitte lässt er sie durchscheinen, ganz unten verschwindet der Kanal. Kanal 1 startet oben und die übrigen unten, also blendet das Hochziehen eines Faders seine Quelle über das Bild ein. Kabel bleiben an ihren Eingängen, wenn du die Anzahl änderst; ein Kabel an einem Eingang, den es bei der neuen Größe nicht gibt, wandert auf einen freien oder wird entfernt.

## Parameter

**Inputs** Wie viele Quellen, von 2 bis 8. Bei zwei ist es der Crossfader weiter unten, bei mehr der Kanalmixer.

**Level 1 to 8** Die Fader des Kanalmixers, einer pro Eingang: wie stark dieser Kanal die Kanäle unter ihm verdeckt.

**Fade** Die Überblendung. 0 ist ganz A (Eingang 1), 1 ist ganz B (Eingang 2), und dazwischen liegt eine Mischung, die Curve formt.

**Curve** Der Überblendverlauf des Crossfaders, derselbe Regler wie bei Crossfader. Bei 0 ist er linear, also stehen auf halbem Weg beide Bilder auf halber Helligkeit, und das Bild wird dunkler. Höhere Werte halten jede Seite länger hell; zwei Drittel ist die Equal-Power-Kurve, und 1 ist die harte Variante.

## Rezept

**Wechsel im Takt** Verbinde einen VideoPlayer mit Eingang 1, ein CameraIn mit Eingang 2 und den Ausgang mit einem VideoOut. Stell Curve auf zwei Drittel und füge einen LFO hinzu, dann rechtsklicke auf Fade, wähle Steuern mit und nimm den LFO, und stell den LFO auf Sync mit 8 Schlägen und Rechteckwelle, damit die Quellen mit dem Takt wechseln.

**Vier Quellen, ein Bildschirm** Stell Inputs auf 4 und verbinde ein CameraIn, zwei VideoPlayer und ein VideoPad mit den Eingängen 1 bis 4. Lass Kanal 1 als Basis oben, weise dann den Fadern 2 bis 4 drei Hardware-Fader zu und schieb jeden hoch, um seine Quelle über die anderen zu blenden.

## Verwandte Organisms

Lumen, VideoPlayer, VideoOut
