# Grit

Ein Chip-Synth auf Basis des Konsolen-Soundchips Ricoh 2A03, mit einem Modulschacht für die Famicom-Erweiterungschips.

Die Konsole allein hat zwei Pulsstimmen mit den vier Hardware-Tastverhältnissen, ein Dreieck, das unter der tiefsten gehaltenen Note liegt, einen Rauschkanal auf den oberen Tasten und einen 1-Bit-Delta-Sampler auf den unteren Tasten. Steck ein Modul ein, und seine Stimmen kommen in den Pool und übernehmen zuerst die Noten, die Pulsstimmen der Konsole springen ein, wenn sie alle belegt sind; beim VRC6 kommt der Sägezahn zuerst. Die Hüllkurve zählt in Frames pro Lautstärkestufe über sechzehn Stufen, sodass ein schneller Attack zuschnappt und ein langsamer hörbar ansteigt. Die Emulation stammt von [NSFPlay](https://github.com/bbbradsmith/nsfplay) und wird Register für Register angesteuert. Verbinde eine PianoRoll, ein Riff oder ein DNA mit dem MIDI-Eingang.

## Parameter

**Cartridge** Der Chip im Schacht. 2A03 ist die nackte Konsole; VRC6 fügt zwei Pulsstimmen und einen Sägezahn hinzu, MMC5 zwei Pulsstimmen, FDS eine Wavetable-Stimme, N163 vier Wavetable-Stimmen, 5B drei feste Rechtecke und VRC7 sechs FM-Stimmen mit je zwei Operatoren. Ein Wechsel lässt gehaltene Noten verstummen.

**Region** NTSC- oder PAL-Konsole. Taktfrequenz, Tonhöhentabellen und Hüllkurven-Tick folgen, deshalb läuft PAL etwas langsamer und dunkler.

**Duty** Pulsbreite für die Pulsstimmen der Konsole, des VRC6 und des MMC5: 12%, 25%, 50% oder 75%.

**Attack** Frames pro Lautstärkestufe auf dem Weg nach oben. 0 startet eine Note mit voller Lautstärke.

**Decay** Frames pro Lautstärkestufe auf dem Weg hinunter zu Sustain.

**Sustain** Der Pegel, auf dem sich eine gehaltene Note einpendelt, 0 bis 15.

**Release** Frames pro Lautstärkestufe, nachdem die Taste losgelassen wurde.

**Triangle** Tri layer: legt den Dreieckkanal der Konsole in gleicher Tonhöhe unter die tiefste gehaltene Note. Er hat keine Lautstärkeregelung, nur an oder aus.

**NoiseKeys** Noise keys: C7 und höher spielen den Rauschkanal statt eines Tons und gehen nach oben hin durch die sechzehn Hardware-Perioden.

**Buzz** Schaltet das Rauschen in seinen Short-Loop-Modus, ein metallisches Rasseln statt eines Rauschteppichs.

**Wave** Die Wavetable, die FDS und N163 spielen: SIN, TRI, SAW, SQR oder ORG.

**Patch** Eines der fünfzehn eingebauten FM-Instrumente des VRC7, nach Nummer.

**Sample** Ein Klang für den Delta-Kanal, auf 1 Bit zerdrückt. Tasten unter C2 lösen ihn mit den sechzehn Hardware-Abspielraten aus, von B1 abwärts tiefer gestimmt.

**DpcmLoop** Loop: lässt das Sample kreisen, solange seine Taste gehalten wird.

**Level** Ausgangspegel.

**BendRange** Wie weit das Pitch-Bend-Rad bei vollem Ausschlag reicht, in Halbtönen. Zwei ist der übliche Standard; null ignoriert das Rad.

## Rezept

**Modul-Lead** Cartridge VRC6, Duty 25%, Attack 0, Decay 6, Sustain 10, Release 3, Tri layer an. Verbinde ein Riff mit dem MIDI-Eingang für die Linie und leg einen Kick mit 4/4 darunter. Zieh einen kurzen Drum-Hit in Sample und spiel ihn ab B1 auf einer PianoRoll.

## Verwandte Organisms

pH, Silt, Riff
