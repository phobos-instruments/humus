# VideoTrack

Eine Videospur auf der Timeline, die ihre Clips zum Transport abspielt und aufnimmt, was an ihrem Eingang ankommt.

Füg eine über das Menü zum Hinzufügen von Spuren in der Timeline hinzu, oder zieh ein Video ins Arrangement, und es erscheint eine Spur dafür. Clips lassen sich schneiden, trimmen, verschieben, loopen und blenden wie Audio-Clips, und der Ausgang zeigt, was gerade unter der Abspielposition liegt: Stoppst du, hält er diesen Frame; springst du an eine Stelle, springt er mit; loopst du den Song, loopt er mit. Warp streckt einen Clip vom Tempo seiner Quelle auf das Tempo des Songs, Rückwärts spielt ihn von hinten nach vorn ab, und Blenden sind Blenden nach Schwarz. Jeder Clip trägt einen Streifen Frames aus seinem Material, und mit einem Rechtsklick auf einen Clip zeigt dir Medieninfo seinen Codec, seine Größe und seine Framerate. Schick den Ausgang in ein VideoOut, ein VideoFX oder eine Lumen-Ebene; zwei Spuren sind zwei Ebenen.

## Aufnahme

Schließ ein CameraIn, eine VideoFX-Kette, ein VideoPad oder eine andere Spur am Eingang an, mach die Spur mit Record scharf und drück am Transport auf Aufnehmen: Was am Eingang ankommt, wird als Video in den Aufnahmeordner geschrieben, solange der Transport läuft, und landet als Clip auf der Timeline, an dem Schlag, an dem es begann, eine Runde pro Durchgang eines Loops. Ton gehört nicht zum Bild; mach daneben eine Audiospur scharf. Die Aufnahmegröße stellst du in den Einstellungen unter Video ein.

## Parameter

**Record** Macht die Spur aufnahmebereit. Solange sie scharf ist, schreibt die nächste Transport-Aufnahme einen Take vom Eingang.

**Monitor** Was der Ausgang zeigt. In zeigt immer den Eingang, Off immer die Clips, und Auto zeigt den Eingang, solange die Spur scharf ist oder der Transport steht, und die Clips, während er läuft. Eine Wahl gibt es nur, solange etwas am Eingang angeschlossen ist: Ist dort nichts, erscheinen die Clips, egal was hier eingestellt ist.

**Opacity** Wie stark das Bild der Spur zu sehen ist. Über Ebenen darunter in einem Lumen oder über einer anderen Quelle am selben Eingang blendet es zwischen ihnen über; gibt es nichts darunter, blendet es das Bild Richtung Schwarz. So oder so wird es genau einmal angewendet, wo immer es landet.

**Blend** Wie es sich mit den Ebenen darunter verbindet: Normal, Add, Multiply oder Screen. Anders als Opacity braucht dieser Parameter etwas darunter, mit dem er sich verbinden kann, deshalb bewirkt er nichts auf einer Spur, die direkt an einem VideoOut oder an einer Seite eines VideoMix hängt.

## Rezept

**Live-Aufnahme über einem Loop** Schließ ein CameraIn am Eingang der Spur an und den Ausgang der Spur an ein VideoOut. Stell Monitor auf Auto, mach Record scharf, loope vier Takte und drück Aufnehmen: Während der Aufnahme siehst du die Kamera, und sobald du stoppst, spielt der Take im Arrangement, auf den Loop zugeschnitten.

## Verwandte Organisms

VideoPad, VideoPlayer, VideoOut, AudioTrack
