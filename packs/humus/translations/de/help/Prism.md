# Prism

Ein harmonischer Resynthesizer, der den Eingang in sechzehn Harmonische zerlegt und jeder einen Fader gibt.

Bei eins geht eine Harmonische so durch, wie sie klingt, bei null wird sie abgezogen, und über eins wird sie verstärkt; das gilt auch für Harmonische, die die Quelle nie hatte; eine hinzugefügte Harmonische folgt der eigenen Hüllkurve des Grundtons, sodass sie mit der Note aufblüht und verklingt. Refract baut den Klang Teilton für Teilton über einem Pitch-Tracker neu auf und will tonales Material, eine Note nach der anderen; Filter ist ein Equalizer für die Harmonischen, direkt auf dem Spektrum, und kommt mit Akkorden, Drums und Rauschen zurecht. Der Direktsignalweg wird passend verzögert, sodass jede Mix-Einstellung phasenkohärent bleibt. Die Anzeige zeigt die Note, auf die der Tracker eingerastet ist, benannt nach dem Tuning des Patches. Füttere es mit einer Stimme, einer Basslinie oder einer Wave.

## Parameter

**Mode** Refract resynthetisiert die Teiltöne in Mono und blendet auf Stille aus, wenn nichts Tonales zu hören ist. Filter formt die schon vorhandenen Harmonischen um, mit unverändertem Stereo, und kann keine neuen hinzufügen.

**Dispersion** Nur Refract. Streckt die Abstände zwischen den Teiltönen, sodass eine Harmonische jenseits ihres ganzzahligen Vielfachen des Grundtons landet; positiv rutscht Richtung Glocken und Metall, ein Hauch negativ zieht den Stapel enger.

**Residual** Nur Filter. Der Gain von allem zwischen den Harmonischen. Bei 0 wird ein Klang auf seinen Ton reduziert; mit den Fadern auf 0 und Residual oben verschwindet der Ton, und Atem und Raum bleiben.

**Mix** Direktsignal gegen Effektsignal.

**Level** Ausgangspegel des Effektsignals.

**Harm_1** Der Fader für die erste Harmonische, 0 bis 2 mit eins in der Mitte, und so weiter für Harm_2 bis Harm_16.

## Rezept

**Glocke aus einem Bass** Mode Refract, und schick ein Rhizome mit einzelnen Noten hinein. Harm_1 auf 1, Harm_2 bis Harm_5 auf 1.6, der Rest auf 0, Dispersion 6, Mix 1. Jede Note klingt jetzt wie eine Glocke, die der eigenen Hüllkurve des Basses folgt.

## Verwandte Organisms

Wave, Decomposer, Tuning
