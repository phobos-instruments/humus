# Math

Eine einzeilige Formel, die einmal pro Sample läuft, als Oszillator, Prozessor oder Modulator.

Zuerst die Lautstärke runter: Math spielt alles, was du tippst, direkt an den Ausgang, und eine Formel kann Vollaussteuerung sein, reiner Gleichanteil oder ein Ton weit außerhalb des Hörbaren. Die beiden Eingänge kommen als a und b an, die vier Regler als x, y, z und w, der MIDI-Eingang als note, freq, gate und vel, und die Clock steuert t, beat, bpm und sr bei. Ein Entwurf, der sich parsen lässt, klingt sofort, und eine halb getippte Änderung lässt den letzten gültigen Ausdruck weiterlaufen. Es gibt zwei Ausgänge: Eine Zeile, die ch nie erwähnt, geht an beide, und eine, die es tut, wird pro Seite berechnet. Der Steuerwert out folgt dem letzten Sample jedes Blocks, deshalb legt "Steuern mit" die Formel auf jeden beliebigen Regler. Verbinde eine PianoRoll mit dem MIDI-Eingang für eine Stimme, oder ein Audiosignal mit a für einen Prozessor.

## Das Vokabular

Zahlen, Klammern, + - * / %, Vergleiche, die 0 oder 1 ergeben, if(c, then, else); nebeneinanderstehende Terme werden multipliziert, also funktioniert 2a. Funktionen: sin cos tan tanh asin exp log log2 sqrt abs floor ceil fract pow min max clamp mix step smoothstep, die Phasenformen saw tri sqr pulse, ph(hz), harm(hz, n, tilt), env(gate, attack, release) und noise(). prev ist das vorige Ausgangssample, dt ist ein Sample in Sekunden, und pi, tau und e sind gesetzt. Anweisungen werden durch Semikolons getrennt, und name := value behält einen Wert zwischen den Samples.

## Parameter

**Expression** Die Formel. a und b sind die Eingänge, x, y, z und w die Regler, ch die Ausgangsseite, und jeder andere Name ist die mit Vars verbundene Var dieses Namens (kleingeschrieben, Leerzeichen als Unterstriche). Sie wird so gespeichert, wie sie geschrieben ist, ob sie sich parsen lässt oder nicht.

**X** Reglervariable x, 0 bis 1. Automatisiere sie, weise sie zu oder morphe sie. Eine mit Vars verbundene Var namens x steuert diesen Regler: Der Regler bewegt sich mit der Var, und der Klang folgt.

**Y** Reglervariable y, 0 bis 1.

**Z** Reglervariable z, 0 bis 1.

**W** Reglervariable w, 0 bis 1.

**Auto** An: Die Formel spielt von selbst, mit freq auf dem Wert des Freq-Reglers und gate auf 1. Aus: Sie klingt nur, solange eine MIDI-Note gehalten wird, auf der Tonhöhe dieser Note.

**Freq** Die Tonhöhe, die freq trägt, solange Auto an ist und keine Taste gedrückt ist, 20 bis 2000 Hz.

**Vars** Ein Eingang für jede Var, die du im Patch zeigen willst. Verbinde eine Var, und ihr Name erledigt den Rest: x, y, z und w steuern ihre Regler, jeder andere Name wird von der Formel gelesen. Nur verbundene Vars zählen: Zieh das Kabel ab, und die Verbindung ist weg.


## Rezept

**Spielbare Stimme** Auto aus, Expression `sin(tau*ph(freq)) * vel * env(gate, 0.01, 0.3)`, und verbinde eine PianoRoll mit dem MIDI-Eingang. Ersetze `sin(tau*ph(freq))` durch `harm(freq, 1 + x*15, y*2)`, und x zählt die Obertöne, während y sie formt.

**Benannte Bedienelemente** Leg eine Var namens x und eine namens depth an, verbinde beide mit Vars und schreib `sin(tau*ph(freq)) * depth * x`. Zieh einen Slider oder einen LFO auf den Value jeder Var: Der X-Regler bewegt sich mit der ersten, die Formel liest die zweite, bis zu acht Namen.

## Verwandte Organisms

Number, LFO, Follower, Var, Wave
