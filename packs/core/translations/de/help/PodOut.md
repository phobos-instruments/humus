# PodOut

Der Weg aus einem Pod hinaus: ein Ausgangs-Pin an der Box des Pods, gespeist von einem Eingang im Inneren.

Alles, was im Pod per Kabel in ein PodOut geht, erscheint draußen am passenden Ausgangs-Pin der Pod-Box, ein PodOut pro Pin, in der Reihenfolge, in der sie angelegt wurden. Füge eines über das Rechtsklick-Menü der Pod-Arbeitsfläche oder aus der Kategorie Pod hinzu. Der Editor des Ports ist ein einziges Dropdown, das ihn an Ort und Stelle zwischen Mono, Stereo, MIDI, Video und Steuerung umstellt; der Port behält seinen Namen, sodass jedes Kabel erhalten bleibt, und die Box des Pods bekommt einen passenden Pin. Ein Steuerungs-PodOut ist das Spiegelbild des Steuerungs-PodIns: Zieh einen beliebigen Steuerausgang im Pod auf seine Buchse, und die Box bekommt einen Steuerausgang, der den Wert nach draußen trägt, sodass ein Pod selbst eine Steuerquelle sein kann. Ein Video-Port trägt Frames statt Klang und gibt sie unverändert weiter.

## Rezept

**Pod als Steuerquelle** Verbinde in einem Pod ein Stereo-PodIn mit einem Follower und zieh den Ausgang env des Followers auf ein Steuerungs-PodOut. Verbinde draußen den Kick mit dem Pod und zieh den neuen Steuerausgang des Pods auf einen Gain am Bass: Der Pod duckt den Bass mit dem, was er gerade hört.

## Verwandte Organisms

PodIn, Follower, Gain
