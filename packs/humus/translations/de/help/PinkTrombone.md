# PinkTrombone

Eine menschliche Stimme aus einem Modell des Rachens, nach [Pink Trombone](https://dood.al/pinktrombone/) von Neil Thapen.

Es gibt kein Sample und keine Oszillatorbank: Ein Modell der Glottis erzeugt das Schnarren der Stimmlippen, und ein Wellenleiter in der Form von Rachen, Mund und Nase filtert es zu Vokalen, Summen und Zischen. Spiel sie über MIDI: Die gehaltene Note ist die Tonhöhe, und die Velocity wirkt auf die Lautstärke. Das Tongue-Paar steuert den Vokal, Squeeze verengt den Trakt bei Place für Reibelaute und Verschlusslaute, und Nasal öffnet den Weg zur Nase. Kleine Schwankungen in Tonhöhe und Druck halten eine gehaltene Note lebendig. Zieh einen LFO oder einen Slider auf TongueX, Squeeze und Nasal, und sie plappert; verbinde ihren Ausgang mit einem Harmonizer oder einem Trellis.

## Parameter

**TongueX** Front. Schiebt den Zungenrücken zwischen Rachen und Zähnen hin und her; zusammen mit TongueY legt er den Vokal fest.

**TongueY** Raise. Wölbt die Zunge zum Gaumen hin.

**Place** Wo der Trakt verengt wird: Lippen am oberen Ende des Reglers, Rachen am unteren.

**Squeeze** Wie stark der Trakt bei Place verengt wird. Gar nicht für Vokale, ein wenig für Reibelaute, ganz zu, um die Luft zu stoppen, sodass beim Loslassen ein Plosiv knallt.

**Nasal** Öffnet das Gaumensegel, sodass Klang durch die Nase entweicht. Halt es offen für ein Summen auf m und n, oder färb einen Vokal mit ein wenig davon.

**Tenseness** Druck der Stimmlippen, von gehauchtem Flüstern bis zu gepresstem Schnarren. Die Velocity skaliert das Ergebnis.

**Vibrato** Tiefe der gewollten Tonhöhenwelle und der langsamen Drift darunter.

**Wobble** Übertreibt das unwillkürliche Wandern zu einem langsamen Leiern.

**Drone** Lässt die Stimme ohne gehaltene Note weiterklingen, auf der letzten Tonhöhe, die sie bekommen hat.

**Width** Verteilt die Stimme über die Stereo-Ausgänge. Bei 0 tragen beide Seiten dieselbe Stimme; aufgedreht driften die Seiten auseinander wie ein Sänger in einem Raum, und zu Mono zusammengemischt bleibt die Stimme unverändert.

**Pan** Setzt die Stimme nach links oder rechts. In der Mitte gehen beide Seiten mit vollem Pegel durch; außerhalb der Mitte wird nur die abgewandte Seite leiser.

**Level** Ausgangs-Gain.

**BendRange** Wie weit das Pitch-Bend-Rad bei vollem Ausschlag reicht, in Halbtönen. Zwei ist der übliche Standard; null ignoriert das Rad.

## Rezept

**Chorteppich** Drone an, Tenseness 0.5, Vibrato 0.3, TongueX 0.3 und TongueY 0.6 für ein U. Schick eine MIDI-Note, um die Tonhöhe festzulegen, zieh einen LFO mit 0.1 Hz und geringer Tiefe auf TongueX, damit der Vokal driftet, und schick den Ausgang durch ein Verbatim.

## Verwandte Organisms

Harmonizer, Trellis, Decomposer
