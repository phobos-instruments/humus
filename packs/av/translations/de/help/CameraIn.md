# CameraIn

Eine Kamera als Videoquelle.

Schalte Enabled ein und verbinde den Videoausgang mit einer Lumen-Ebene, einem VideoMix, einem VideoFX oder einem VideoOut, und das Livebild wird zu einer Ebene wie jedes Videoband. Die Kamera ist nur geöffnet, solange Enabled an ist, und schließt sich, sobald es ausgeht; die Frames gehen an die Anzeige und sonst nirgendwohin. Sie liefert ein Bild, keinen Controller: Um mit einer Kamera etwas zu spielen, verbinde sie mit Hands oder Skeleton, oder lass die beiden die Kamera selbst öffnen.

## Parameter

**Enabled** Öffnet die Kamera. Aus schließt das Gerät, statt nur das Bild zu verstecken.

**Camera** Welches Aufnahmegerät geöffnet wird, wenn der Rechner mehr als eines hat.

**Mirror** Spiegelt das Bild von links nach rechts, sodass du dich auf dem Bildschirm nach links bewegst, wenn du dich nach links lehnst. Schalte es ein, wenn du vor der Kamera performst.

**Opacity** Wie stark diese Ebene in einem Lumen über den Ebenen darunter zu sehen ist.

**Blend** Wie sich die Ebene mit den Ebenen darunter verbindet: Normal, Add, Multiply oder Screen.

## Rezept

**Kamera unter einer Szene** Enabled an, Mirror an, dann verbinde den Videoausgang mit dem ersten Videoeingang von Lumen und Lumen mit einem VideoOut. Lade einen Szenen-Shader in Lumen, setz SceneBlend auf Add und SceneOpacity auf etwa 0.6, damit die Szene über dem Livebild leuchtet.

## Verwandte Organisms

Lumen, VideoMix, VideoOut, Hands
