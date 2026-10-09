# Wave

Ein Wavetable-Oszillator, dessen Zyklen gezeichnet, aus einer Vorlagedatei gewachsen oder zufällig gewürfelt sind.

Die Anzeige ist der Klang: Zeichne darauf, starte mit der Sinus-, Dreieck-, Sägezahn- oder Rechteck-Kachel oder nimm eine beliebige Datei als Vorlage für die Tabelle. Eine Tabelle enthält bis zu sechzehn Frames, und Position morpht zwischen den zwei nächsten; Warp formt den Zyklus um, Unison stapelt verstimmte Kopien, und ein Sub-Sinus, Drive und ein Tiefpass runden die Stimme ab. Die Wiedergabe ist pro Frame bandbegrenzt, sodass hohe Noten nur die Obertöne verlieren, die nicht hineinpassen. Es ist ein achtstimmiges MIDI-Instrument, das dem Tuning des Patches folgt; verbinde eine PianoRoll, ein DNA oder ein Steps mit dem Eingang und schick den Ausgang an einen Filter oder ein Fern.

## Die Anzeige

Zieh auf der Welle, um das Frame zu zeichnen, das Position am nächsten liegt; der Rest einer Tabelle aus einer Vorlage bleibt unberührt, sodass eine gewachsene Welle Frame für Frame nachbearbeitet werden kann. Zufall würfelt eine frische Verteilung von Teiltönen. Zieh eine Datei auf die Anzeige oder nutze Vorlagedatei: Eine Wavetable-Datei, ein Zyklus von 2048 Samples pro Frame, wie die meisten Synths sie teilen, wird Frame für Frame gelesen, und eine lange wird auf sechzehn Frames ausgedünnt, die ihr erstes und letztes behalten; eine Aufnahme wird in bis zu sechzehn Frames zerhackt, ein Zyklus pro Slice; und jede andere Datei wird als Form gelesen. + Frame und - Frame bauen eine Tabelle von Hand, und der Streifen unter der Anzeige zeigt jedes Frame, das aktuelle hervorgehoben; klick auf ein Slice, um dorthin zu springen. Dieselbe Vorlage lässt immer dieselbe Tabelle wachsen, gespeichert mit dem Patch.

## Parameter

**Attack** Wie lange eine Note zum Öffnen braucht, in Millisekunden.

**Release** Wie lange sie zum Ausklingen braucht, nachdem die Taste losgelassen wurde.

**Sub** Ein reiner Sinus eine Oktave unter jeder Stimme, für Gewicht, ohne den gezeichneten Zyklus zu verändern. 0 ist aus.

**Unison** Wie viele verstimmte Kopien jede Stimme spielt, 1 bis 16. Eins ist genau mittig; mehr macht dicker und breiter, und über acht zahlst du in CPU statt in Klang.

**Detune** Wie weit die Unison-Kopien in der Tonhöhe auseinanderliegen.

**Width** Wie weit sich die Unison-Kopien im Stereofeld verteilen.

**Cutoff** Ein Tiefpass über die summierten Stimmen. Ganz oben geparkt, verlässt er den Signalweg vollständig.

**Reso** Resonanz dieses Tiefpasses.

**Drive** Sättigung auf der Summe, vor dem Filter, sodass Cutoff zähmt, was Drive erzeugt. 0 ist sauber.

**WarpMode** Der Zyklus-Former: Off, Sync liest innerhalb der Periode schneller, Bend verzerrt die Phasenrampe, Fold faltet die Spitzen in den Bereich zurück, PWM staucht das Tastverhältnis.

**Warp** Tiefe des Warps. Er wird geglättet, sodass ein Sweep nie zippert, und das geformte Ergebnis wird als zweite Linie über den Grundzyklus gezeichnet.

**Position** Durchläuft die Frames der Tabelle und morpht zwischen ihnen. Wirkt nicht bei einer Tabelle mit nur einem Frame.

**Spectral** Morpht nach Obertonstärke statt nach Samples, sodass ein Teilton, der zwischen zwei Frames gegenphasig liegt, ausgeblendet wird, statt sich mitten im Sweep auszulöschen.

**PositionMod** Wie weit die Amp-Hüllkurve Position von selbst bewegt, pro Stimme, sodass eine Note beim Spielen durch die Tabelle wandert. 0 ist aus.

**Level** Ausgangspegel.

**BendRange** Wie weit das Pitch-Bend-Rad bei vollem Ausschlag reicht, in Halbtönen. Zwei ist der übliche Standard; null ignoriert das Rad.

**Table** Die Tabelle selbst, gespeichert mit dem Patch. Zufall würfelt sie zusammen mit den Formreglern neu, und Widerrufen holt die Welle zurück, die du hattest.

## Rezept

**Wandelnde Sample-Tabelle** Vorlagedatei mit einer kurzen Gesangsaufnahme, Unison 3, Detune 0.25, Width 0.7, Cutoff 6000, Drive 0.2, Release 800. Verbinde eine PianoRoll mit dem Eingang und einen LFO mit Position, mit einer Periode von zwei Takten, sodass jede Note beim Spielen durch die Aufnahme wandert; schalte Spectral an, wenn der Sweep einbricht oder phasig klingt.

## Verwandte Organisms

Rhizome, Prism, Tuning
