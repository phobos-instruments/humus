# Mineral

Ein Synth für angeschlagene Perkussion, dessen Teiltöne auf einem geometrischen Gitter sitzen, jeder der vorige mal Ratio.

Diese Reihe ist das inharmonische Spektrum von angeschlagenem Glas, Glocken und Gamelan-Metall, und die Form des Gitters ist die Klangfarbe. Vier Stimmen spielen vom MIDI-Eingang, also sequenziere es mit einer PianoRoll oder einem DNA, oder spiel es über die Tastatur, solange sein Editor den Fokus hat. Das Siegel links im Editor zeichnet das Modell live: eine Seite pro Facette, gestreckt von Ratio, verdoppelt von Shimmer, aufblitzend bei jedem Anschlag. Verbinde den Ausgang mit einem Verbatim oder einem Fern für den Raum, den eine Glocke braucht.

## Parameter

**Facets** Wie viele Teiltöne, 1 bis 12. Wenige ergeben einen angeschlagenen Stab; viele füllen sich zu einer Glocke.

**Ratio** Die Gitterkonstante. 2.0 sind Oktaven, 1.62 die klassische Inharmonizität einer Glocke, niedriger klingt geballt und metallisch.

**Geometry** Überlässt Ratio der Facettenzahl: die Diagonale eines regelmäßigen Polygons geteilt durch seine Seite, sodass fünf Facetten 1.618 ergeben und mehr Facetten Richtung Oktave steigen. Der Ratio-Regler wird ausgegraut, solange es an ist.

**Shine** Pegelverlauf über die Facetten, von dumpf bis gläsern.

**Decay** Ausklingzeit in Millisekunden. Höhere Facetten klingen immer schneller aus.

**Strike** Der Anschlagtransient, ein kurzer Rauschstoß.

**Shimmer** Verstimmt jede Facette links und rechts leicht unterschiedlich, sodass der Klang über das Stereofeld schwebt.

**Level** Ausgangspegel.

**BendRange** Wie weit das Pitch-Bend-Rad bei vollem Ausschlag reicht, in Halbtönen. Zwei ist der übliche Standard; null ignoriert das Rad.

## Rezept

**Gamelan** Facets 6, Ratio 1.62, Shine 0.75, Decay 1200, Strike 0.5, Shimmer 0.4. Verbinde ein DNA mit dem MIDI-Eingang für eine pentatonische Linie und den Ausgang mit einem Verbatim mit langer Fahne.

## Verwandte Organisms

DNA, Rhizome, PianoRoll
