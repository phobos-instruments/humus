# Lumen

Ein Shader-Compositor, der das Audio malt, das er hört, und Videoebenen darunter stapelt.

Schick Audio in seine zwei Eingänge, und die Szene zeichnet dazu; verbinde seinen Videoausgang mit einem VideoOut, um sie zu sehen, so wie ein Synth ein SoundOut braucht, um gehört zu werden. Scene nimmt eine Fragment-Shader-Datei und lädt sie neu, sobald sich die Datei auf der Festplatte ändert; bleibt es leer, läuft die eingebaute Szene. Shader lesen das Audio als Pegel, Bänder, Schlag, Tempo, eine Wellenform-Textur und eine Spektrum-Textur, und die eigenen Float-Eingänge einer Szene hängen an Knob1 bis Knob4. Eine Szene kann auch die Videoebenen unter ihr und ihren eigenen letzten Frame lesen und so als Feedback-Effekt arbeiten: Die mitgelieferte Szene hallucination träumt über alles, was du hineinschickst. Die vier Videoeingänge nehmen Ebenen von VideoPlayer, VideoPad, CameraIn oder VideoTrack, von unten nach oben gestapelt, und die Szene schwebt mit SceneOpacity und SceneBlend darüber. Deckkraft und Blendmodus jeder Ebene liegen beim Organism, das sie speist.

## Parameter

**Scene** Die Shader-Datei, die laufen soll: .frag, .fs, .glsl oder .fsh. Bleibt es leer, läuft die eingebaute Szene, und Änderungen an der Datei werden während der Wiedergabe neu geladen.

**Brightness** Der Gesamtausgangspegel der Szene.

**Speed** Wie schnell die eigene Uhr der Szene läuft. 0 friert sie ein; 2 läuft doppelt so schnell.

**Knob1** Ein Wert, den die Szene für sich liest; was er bewirkt, hängt vom Shader ab. Ein Effekt-Shader nimmt hier seine Stärke.

**Knob2** Der zweite solche Wert. Ein Effekt-Shader nimmt hier Geschwindigkeit und Farbe.

**Knob3** Der dritte. Ein Effekt-Shader nimmt hier Frequenz, Dichte und Rasteranzahl.

**Knob4** Der vierte. Ein Effekt-Shader nimmt hier Breite und Detail.

**SceneOpacity** Wie stark die Szene über den Videoebenen darunter zu sehen ist.

**SceneBlend** Over legt die Szene über die Videoebenen. Add addiert sie zu ihnen, sodass die dunklen Teile der Szene verschwinden und die hellen leuchten.

## Rezept

**Audioreaktives Overlay** Schick die Master-Summe in beide Audioeingänge und einen VideoPlayer in den ersten Videoeingang. Lass Scene leer, stell SceneBlend auf Add und SceneOpacity auf 0.5 und leg dann einen Follower vom Kick auf Brightness, damit die Szene mit jedem Schlag aufblitzt. Verbinde den Ausgang mit einem VideoOut und öffne es über das Menü Videoausgänge.

## Verwandte Organisms

VideoPlayer, VideoMix, VideoOut
