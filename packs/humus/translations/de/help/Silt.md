# Silt

Ein dreistimmiger Chip-Synth, der den MOS SID, den Soundchip des Heimcomputers, über den Kern [reSID](https://github.com/daglem/reSID) emuliert.

Sowohl der ursprüngliche 6581 als auch die sauberere Revision 8580 werden Zyklus für Zyklus emuliert, samt analogem Filter. Drei Stimmen teilen sich einen Filter, wie bei der Hardware; eine vierte Note stiehlt die älteste Stimme. Jede Stimme spielt dieselbe Wellenform, dieselbe Pulsbreite und die chipeigene vierstufige Hüllkurve in ihren nativen sechzehn Stufen. Die Tonhöhe kommt aus der Stimmung des Patches, so fein, wie die Frequenzregister des Chips auflösen, sodass auch eine mikrotonale Tonleiter spielt. Der Twin-Modus fügt einen zweiten Chip hinzu, wie bei den umgebauten Boards, und doppelt jede Note, wobei das Paar über das Stereobild verstimmt ist. Spiel es über ein MIDI-Kabel von PianoRoll, DNA oder MidiIn, oder mit der Tastatur, während sein Editor im Fokus ist.

## Parameter

**Model** Welcher Chip: 6581 mit dem warmen, eigenwilligen Filter oder 8580 mit dem korrigierten. Beim Umschalten blendet der Pegel kurz ein, um den Sprung zwischen den Ruhespannungen der beiden Chips abzumildern.

**Wave** TRI, SAW, PLS oder NSE, die vier Wellenformen des Chips.

**PulseWidth** Das Tastverhältnis der Pulswelle. Nur bei PLS hörbar.

**Attack** Attack der Hüllkurve in den sechzehn Stufen des Chips. 0 ist sofort.

**Decay** Decay der Hüllkurve bis zum Sustain-Pegel, in sechzehn Stufen.

**Sustain** Der Pegel, der gehalten wird, solange die Note gedrückt ist, in sechzehn Stufen.

**Release** Release der Hüllkurve nach dem Note-off, in sechzehn Stufen. 15 klingt sekundenlang nach.

**Cutoff** Wo der Filter schließt, über den Registerbereich der Hardware.

**Resonance** Wie stark der Filter an der Cutoff-Frequenz singt, in sechzehn Stufen.

**FilterMode** LOW, BAND oder HIGH, also Tief-, Band- oder Hochpass.

**Filter** Nimmt den Filter ganz heraus, für die rohen Stimmen, heller und lauter.

**Ring** Jede Stimme wird mit ihrer Nachbarin ringmoduliert, metallisch und scheppernd. Hörbar wird das über die TRI-Welle.

**Sync** Jede Stimme wird hart auf ihre Nachbarin synchronisiert.

**Chips** ONE Chip oder TWIN: jede Note auf beiden Chips, wobei jeder eine Seite anführt und etwas vom anderen beigemischt bekommt.

**TwinDetune** Wie weit das Twin-Paar gegeneinander verstimmt ist, in Cent, links und rechts der Note. Angezeigt als Spread.

**Chain** Führt den Ausgang des ersten Chips durch den Filter des zweiten, sodass die linke Seite unbearbeitet ankommt und die rechte durch einen zweiten analogen Filter. Nur bei TWIN.

**ModelB** Welcher Chip der zweite ist, B 6581 oder B 8580. Ein warmes Original mit der sauberen Revision zu mischen, ist der halbe Sinn von zwei Chips.

**Level** Ausgangspegel.

**BendRange** Wie weit das Pitch-Rad bei vollem Ausschlag reicht, in Halbtönen. Zwei ist der übliche Standard; null ignoriert das Rad.

## Rezept

**Chip-Bass** Model 6581, Wave PLS, PulseWidth 0.25, Attack 0, Decay 9, Sustain 6, Release 5, FilterMode LOW, Cutoff 0.45, Resonance 8. Verkable ein Riff mit dem MIDI-Eingang und leg einen LFO mit einem Viertel Takt auf PulseWidth, für den klassischen Pulsbreiten-Sweep. Schalte auf TWIN mit TwinDetune 8, um den Klang breiter zu machen.

## Verwandte Organisms

pH, Acid, Rhizome, PianoRoll
