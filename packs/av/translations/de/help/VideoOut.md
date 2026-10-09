# VideoOut

Das Fenster oder der Bildschirm, an dem das Bild das Patch verlässt.

Der Video-Zwilling von SoundOut. Verbinde ein Lumen-Composite, einen VideoMix, ein VideoFX oder einen einfachen VideoPlayer mit seinem Eingang und öffne sein Fenster mit der Bildschirm-Schaltfläche neben dem Screen-Menü, über das Untermenü Videoausgänge oder per Rechtsklick auf das Organism. Als Fenster schwebt es über dem Patcher, während du arbeitest; auf einen Bildschirm geschickt, läuft es dort randlos im Vollbild. Mehrere VideoOut-Organisms sind mehrere Ausgänge, so kann die Show auf einen Projektor gehen, während ein Vorschau-Mix auf deinem eigenen Bildschirm bleibt.

## Parameter

**Screen** Wohin das Bild geht. Window öffnet ein schwebendes Fenster, das du verschieben und in der Größe ändern kannst; Display 1, Display 2 oder Display 3 übernimmt den angeschlossenen Bildschirm im Vollbild, das ist die Einstellung für den Projektor.

**Fade** Der Master-Fader für den ganzen Ausgang. 0 ist schwarz; fahr ihn per Automation oder mit einem Controller, um die Show auszublenden.

**Preview** Hält die kleine Vorschau im Editor des Organisms aktiv, solange das Ausgabefenster geschlossen ist. Schalte sie aus, um Grafikarbeit zu sparen, wenn du sie nicht brauchst.

Ein Ausgang, der auf einen Bildschirm gestellt ist, öffnet sich beim Laden des Patches von selbst wieder, auf diesem Bildschirm und bildschirmfüllend. Einer, der auf Window steht, tut das nicht: Im Patch steht nicht, ob sein Fenster offen war, nur wohin es gehen würde.

**OnTop** Hält das Ausgabefenster über den anderen Fenstern, damit ein Klick zurück ins Patch das Bild nicht verdeckt. Standardmäßig an; ein Ausgang, der einen Bildschirm füllt, bleibt immer oben, egal, was hier eingestellt ist.

## Rezept

**Projektor plus Vorschau** Verbinde den Lumen-Ausgang mit zwei VideoOut-Organisms. Stell eines auf Display 2 für den Projektor und lass das andere auf Window auf deinem eigenen Bildschirm, mit Preview an. Weise Fade am Projektor-Ausgang einem Fader zu, damit du den Raum in Schwarz tauchen kannst, ohne den Mix anzufassen.

## Verwandte Organisms

Lumen, VideoMix, VideoPlayer
