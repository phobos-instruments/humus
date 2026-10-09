# VideoPad

Acht Clip-Pads mit einer Überblendung zwischen den Starts.

Jedes Pad enthält einen Clip: Zieh ein Video darauf oder klick auf den Ordner, um eines zu laden, klick auf das Bild, um es zu starten, und der nächste Start blendet über die Fade-Zeit in ihn über. Der MIDI-Eingang startet Pads per Note, das mittlere C für Pad 1 und die Halbtöne darüber für die übrigen; startest du ein Pad, das schon läuft, beginnt es wieder bei In. Zufall würfelt für jedes geladene Pad ein frisches In, Out und Loop und startet eines. Verbinde den Hauptausgang mit einer Lumen-Ebene, einem VideoFX, einem VideoMix oder einem VideoOut.


Die Pads tragen ihren Ton ebenso wie ihr Bild. Ein gestarteter Clip spielt seinen eigenen Ton über den Stereoausgang, ab seinem In-Punkt, loopt zwischen In und Out, wenn dieses Pad loopt, und übergibt an das nächste Pad mit derselben Blende, mit der das Bild wechselt. Mute nimmt beides weg. Das Bild folgt dem Ton, statt neben ihm herzulaufen, sodass ein Schnitt in beiden zugleich landet, statt über einen langen Clip auseinanderzudriften - und ein Pad, das du drückst, während es schon läuft, springt zu seinem In-Punkt zurück, Ton und Bild zusammen.

## Pad-Bearbeitung

Die obere Linie unter jedem Bild ist das ganze Band, mit dem Abschnitt von In bis Out hervorgehoben; die untere Linie ist der Ausschnitt mit seiner Abspielposition. Zieh eine Marke, um einen Punkt zu verschieben, zieh über das Bild, um zu scrubben, und drück In oder Out, um die Abspielposition ins Pad zu stempeln. Zieh ein Pad an seinem Nummernfeld auf ein anderes Pad, um es zu kopieren, oder als Clip auf die Timeline.

## Parameter

**File1** Der Clip auf Pad 1, und so weiter für 2 bis 8.

**Launch1** Startet Pad 1. Tastend; ein Rechtsklick auf ein Pad bietet Start-Steuerung, um es zuzuweisen, und so weiter für 2 bis 8.

**In1** Wo der Clip von Pad 1 beginnt, in Sekunden. Wird von Zufall gewürfelt, und so weiter für 2 bis 8.

**Out1** Wo der Clip von Pad 1 endet, in Sekunden; 0 läuft bis zum Ende der Datei. Wird von Zufall gewürfelt, und so weiter für 2 bis 8.

**Loop1** Ob Pad 1 zwischen In und Out kreist oder einmal durchspielt und sein letztes Frame hält. Wird von Zufall gewürfelt, und so weiter für 2 bis 8.

**Stop** Blendet das Bild über die Fade-Zeit zu Schwarz.

**Mute** Hält den Ausgang schwarz, solange es an ist, ein Blackout-Schalter, der die Clips darunter weiterlaufen lässt.

**Fade** Wie lange eine Überblendung dauert, in Sekunden. 0 ist ein harter Schnitt.

**Rate** Der Speed-Regler, in Prozent, für alle Pads auf einmal. 100% ist wie aufgenommen, 0% hält ein Frame fest.

**Pads** 4 pads oder 8 pads.

**Outlets** One out führt nur den gestarteten Mix. Per pad fügt nach dem Hauptausgang einen Videoausgang für jedes Pad hinzu, sodass ein einzelnes Pad sein eigenes VideoFX oder seinen eigenen Tracker speisen kann.

**Opacity** Wie stark der Hauptausgang sichtbar ist, wenn er eine Lumen-Ebene speist.

**Blend** Wie er sich dort verbindet: Normal, Add, Multiply oder Screen.

## Rezept

**Drum-Pad-Schnitte** Lade vier Clips, stell Pads auf 4 pads und Fade auf 0.1. Verbinde eine PianoRoll oder ein MidiIn mit dem MIDI-Eingang und spiel das mittlere C und die drei Halbtöne darüber, um auf den Schlag zwischen ihnen zu schneiden. Verbinde den Ausgang mit einem VideoOut.

## Verwandte Organisms

VideoPlayer, VideoFX, VideoMix, Lumen
